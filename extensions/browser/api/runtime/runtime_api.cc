// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/runtime/runtime_api.h"

#include <memory>
#include <utility>

#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "base/version.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/api/runtime/runtime_api_delegate.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/lazy_background_task_queue.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/process_manager_factory.h"
#include "extensions/common/api/runtime.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "url/gurl.h"

using content::BrowserContext;

namespace extensions {

namespace runtime = api::runtime;

namespace {

const char kNoBackgroundPageError[] = "You do not have a background page.";
const char kPageLoadError[] = "Background page failed to load.";
const char kFailedToCreateOptionsPage[] = "Could not create an options page.";
const char kInstallId[] = "id";
const char kInstallReason[] = "reason";
const char kInstallReasonChromeUpdate[] = "chrome_update";
const char kInstallReasonUpdate[] = "update";
const char kInstallReasonInstall[] = "install";
const char kInstallReasonSharedModuleUpdate[] = "shared_module_update";
const char kInstallPreviousVersion[] = "previousVersion";
const char kInvalidUrlError[] = "Invalid URL: \"*\".";
const char kPlatformInfoUnavailable[] = "Platform information unavailable.";

const char kUpdatesDisabledError[] = "Autoupdate is not enabled.";

// A preference key storing the url loaded when an extension is uninstalled.
const char kUninstallUrl[] = "uninstall_url";

// A preference key storing the information about an extension that was
// installed but not loaded. We keep the pending info here so that we can send
// chrome.runtime.onInstalled event during the extension load.
const char kPrefPendingOnInstalledEventDispatchInfo[] =
    "pending_on_installed_event_dispatch_info";

// Previously installed version number.
const char kPrefPreviousVersion[] = "previous_version";

// The name of the directory to be returned by getPackageDirectoryEntry. This
// particular value does not matter to user code, but is chosen for consistency
// with the equivalent Pepper API.
const char kPackageDirectoryPath[] = "crxfs";

// Preference key for storing the last successful restart due to a call to
// chrome.runtime.restartAfterDelay().
constexpr char kPrefLastRestartAfterDelayTime[] =
    "last_restart_after_delay_time";
// Preference key for storing whether the most recent restart was due to a
// successful call to chrome.runtime.restartAfterDelay().
constexpr char kPrefLastRestartWasDueToDelayedRestartApi[] =
    "last_restart_was_due_to_delayed_restart_api";

// Error and status messages strings for the restartAfterDelay() API.
constexpr char kErrorInvalidArgument[] = "Invalid argument: *.";
constexpr char kErrorOnlyKioskModeAllowed[] =
    "API available only for ChromeOS kiosk mode.";
constexpr char kErrorOnlyFirstExtensionAllowed[] =
    "Not the first extension to call this API.";
constexpr char kErrorInvalidStatus[] = "Invalid restart request status.";
constexpr char kErrorRequestedTooSoon[] =
    "Restart was requested too soon. It was throttled instead.";

constexpr int kMinDurationBetweenSuccessiveRestartsHours = 3;

// This is used for unit tests, so that we can test the restartAfterDelay
// API without a kiosk app.
bool allow_non_kiosk_apps_restart_api_for_test = false;

void DispatchOnStartupEventImpl(BrowserContext* browser_context,
                                const std::string& extension_id,
                                bool first_call,
                                ExtensionHost* host) {
  // A NULL host from the LazyBackgroundTaskQueue means the page failed to
  // load. Give up.
  if (!host && !first_call)
    return;

  // Don't send onStartup events to incognito browser contexts.
  if (browser_context->IsOffTheRecord())
    return;

  if (ExtensionsBrowserClient::Get()->IsShuttingDown() ||
      !ExtensionsBrowserClient::Get()->IsValidContext(browser_context))
    return;
  ExtensionSystem* system = ExtensionSystem::Get(browser_context);
  if (!system)
    return;

  // If this is a persistent background page, we want to wait for it to load
  // (it might not be ready, since this is startup). But only enqueue once.
  // If it fails to load the first time, don't bother trying again.
  const Extension* extension =
      ExtensionRegistry::Get(browser_context)->enabled_extensions().GetByID(
          extension_id);
  if (extension && BackgroundInfo::HasPersistentBackgroundPage(extension) &&
      first_call &&
      LazyBackgroundTaskQueue::Get(browser_context)
          ->ShouldEnqueueTask(browser_context, extension)) {
    LazyBackgroundTaskQueue::Get(browser_context)
        ->AddPendingTask(browser_context, extension_id,
                         base::Bind(&DispatchOnStartupEventImpl,
                                    browser_context, extension_id, false));
    return;
  }

  std::unique_ptr<base::ListValue> event_args(new base::ListValue());
  std::unique_ptr<Event> event(new Event(events::RUNTIME_ON_STARTUP,
                                         runtime::OnStartup::kEventName,
                                         std::move(event_args)));
  EventRouter::Get(browser_context)
      ->DispatchEventToExtension(extension_id, std::move(event));
}

void SetUninstallURL(ExtensionPrefs* prefs,
                     const std::string& extension_id,
                     const std::string& url_string) {
  prefs->UpdateExtensionPref(
      extension_id, kUninstallUrl, new base::StringValue(url_string));
}

std::string GetUninstallURL(ExtensionPrefs* prefs,
                            const std::string& extension_id) {
  std::string url_string;
  prefs->ReadPrefAsString(extension_id, kUninstallUrl, &url_string);
  return url_string;
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////

static base::LazyInstance<BrowserContextKeyedAPIFactory<RuntimeAPI> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<RuntimeAPI>* RuntimeAPI::GetFactoryInstance() {
  return g_factory.Pointer();
}

// static
void RuntimeAPI::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kPrefLastRestartWasDueToDelayedRestartApi,
                                false);
  registry->RegisterDoublePref(kPrefLastRestartAfterDelayTime, 0.0);
}

template <>
void BrowserContextKeyedAPIFactory<RuntimeAPI>::DeclareFactoryDependencies() {
  DependsOn(ProcessManagerFactory::GetInstance());
}

RuntimeAPI::RuntimeAPI(content::BrowserContext* context)
    : browser_context_(context),
      extension_registry_observer_(this),
      process_manager_observer_(this),
      minimum_duration_between_restarts_(base::TimeDelta::FromHours(
          kMinDurationBetweenSuccessiveRestartsHours)),
      dispatch_chrome_updated_event_(false),
      did_read_delayed_restart_preferences_(false),
      was_last_restart_due_to_delayed_restart_api_(false),
      weak_ptr_factory_(this) {
  // RuntimeAPI is redirected in incognito, so |browser_context_| is never
  // incognito.
  DCHECK(!browser_context_->IsOffTheRecord());

  registrar_.Add(this,
                 extensions::NOTIFICATION_EXTENSIONS_READY_DEPRECATED,
                 content::Source<BrowserContext>(context));
  extension_registry_observer_.Add(ExtensionRegistry::Get(browser_context_));
  process_manager_observer_.Add(ProcessManager::Get(browser_context_));

  delegate_ = ExtensionsBrowserClient::Get()->CreateRuntimeAPIDelegate(
      browser_context_);

  // Check if registered events are up to date. We can only do this once
  // per browser context, since it updates internal state when called.
  dispatch_chrome_updated_event_ =
      ExtensionsBrowserClient::Get()->DidVersionUpdate(browser_context_);
}

RuntimeAPI::~RuntimeAPI() {
}

void RuntimeAPI::Observe(int type,
                         const content::NotificationSource& source,
                         const content::NotificationDetails& details) {
  DCHECK_EQ(extensions::NOTIFICATION_EXTENSIONS_READY_DEPRECATED, type);
  // We're done restarting Chrome after an update.
  dispatch_chrome_updated_event_ = false;

  delegate_->AddUpdateObserver(this);
}

void RuntimeAPI::OnExtensionLoaded(content::BrowserContext* browser_context,
                                   const Extension* extension) {
  base::Version previous_version;
  if (ReadPendingOnInstallInfoFromPref(extension->id(), &previous_version)) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&RuntimeEventRouter::DispatchOnInstalledEvent,
                   browser_context_, extension->id(), previous_version, false));
    RemovePendingOnInstallInfoFromPref(extension->id());
  }

  if (!dispatch_chrome_updated_event_)
    return;

  // Dispatch the onInstalled event with reason "chrome_update".
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&RuntimeEventRouter::DispatchOnInstalledEvent,
                 browser_context_, extension->id(), Version(), true));
}

void RuntimeAPI::OnExtensionWillBeInstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    bool is_update,
    const std::string& old_name) {
  // This extension might be disabled before it has a chance to load, e.g. if
  // the extension increased its permissions. So instead of trying to send the
  // onInstalled event here, we remember the fact in prefs and fire the event
  // when the extension is actually loaded.
  StorePendingOnInstallInfoToPref(extension);
}

void RuntimeAPI::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UninstallReason reason) {
  RemovePendingOnInstallInfoFromPref(extension->id());

  RuntimeEventRouter::OnExtensionUninstalled(
      browser_context_, extension->id(), reason);
}

void RuntimeAPI::Shutdown() {
  delegate_->RemoveUpdateObserver(this);
}

void RuntimeAPI::OnAppUpdateAvailable(const Extension* extension) {
  RuntimeEventRouter::DispatchOnUpdateAvailableEvent(
      browser_context_, extension->id(), extension->manifest()->value());
}

void RuntimeAPI::OnChromeUpdateAvailable() {
  RuntimeEventRouter::DispatchOnBrowserUpdateAvailableEvent(browser_context_);
}

void RuntimeAPI::OnBackgroundHostStartup(const Extension* extension) {
  RuntimeEventRouter::DispatchOnStartupEvent(browser_context_, extension->id());
}

bool RuntimeAPI::ReadPendingOnInstallInfoFromPref(
    const ExtensionId& extension_id,
    base::Version* previous_version) {
  ExtensionPrefs* prefs = ExtensionPrefs::Get(browser_context_);
  DCHECK(prefs);

  const base::DictionaryValue* info = nullptr;
  if (!prefs->ReadPrefAsDictionary(
          extension_id, kPrefPendingOnInstalledEventDispatchInfo, &info)) {
    return false;
  }

  std::string previous_version_string;
  info->GetString(kPrefPreviousVersion, &previous_version_string);
  // |previous_version_string| can be empty.
  *previous_version = base::Version(previous_version_string);
  return true;
}

void RuntimeAPI::RemovePendingOnInstallInfoFromPref(
    const ExtensionId& extension_id) {
  ExtensionPrefs* prefs = ExtensionPrefs::Get(browser_context_);
  DCHECK(prefs);

  prefs->UpdateExtensionPref(extension_id,
                             kPrefPendingOnInstalledEventDispatchInfo, nullptr);
}

void RuntimeAPI::StorePendingOnInstallInfoToPref(const Extension* extension) {
  ExtensionPrefs* prefs = ExtensionPrefs::Get(browser_context_);
  DCHECK(prefs);

  // |pending_on_install_info| currently only contains a version string. Instead
  // of making the pref hold a plain string, we store it as a dictionary value
  // so that we can add more stuff to it in the future if necessary.
  std::unique_ptr<base::DictionaryValue> pending_on_install_info(
      new base::DictionaryValue());
  base::Version previous_version =
      delegate_->GetPreviousExtensionVersion(extension);
  pending_on_install_info->SetString(
      kPrefPreviousVersion,
      previous_version.IsValid() ? previous_version.GetString() : "");
  prefs->UpdateExtensionPref(extension->id(),
                             kPrefPendingOnInstalledEventDispatchInfo,
                             pending_on_install_info.release());
}

void RuntimeAPI::ReloadExtension(const std::string& extension_id) {
  delegate_->ReloadExtension(extension_id);
}

bool RuntimeAPI::CheckForUpdates(
    const std::string& extension_id,
    const RuntimeAPIDelegate::UpdateCheckCallback& callback) {
  return delegate_->CheckForUpdates(extension_id, callback);
}

void RuntimeAPI::OpenURL(const GURL& update_url) {
  delegate_->OpenURL(update_url);
}

bool RuntimeAPI::GetPlatformInfo(runtime::PlatformInfo* info) {
  return delegate_->GetPlatformInfo(info);
}

bool RuntimeAPI::RestartDevice(std::string* error_message) {
  if (was_last_restart_due_to_delayed_restart_api_ &&
      (ExtensionsBrowserClient::Get()->IsRunningInForcedAppMode() ||
       allow_non_kiosk_apps_restart_api_for_test)) {
    // We don't allow an app by calling chrome.runtime.restart() to clear the
    // throttle enforced on it when calling chrome.runtime.restartAfterDelay(),
    // i.e. the app can't unthrottle itself.
    // When running in forced kiosk app mode, we assume the following restart
    // request will succeed.
    PrefService* pref_service =
        ExtensionsBrowserClient::Get()->GetPrefServiceForContext(
            browser_context_);
    DCHECK(pref_service);
    pref_service->SetBoolean(kPrefLastRestartWasDueToDelayedRestartApi, true);
  }
  return delegate_->RestartDevice(error_message);
}

RuntimeAPI::RestartAfterDelayStatus RuntimeAPI::RestartDeviceAfterDelay(
    const std::string& extension_id,
    int seconds_from_now) {
  // To achieve as much accuracy as possible, record the time of the call as
  // |now| here.
  const base::Time now = base::Time::NowFromSystemTime();

  if (schedule_restart_first_extension_id_.empty()) {
    schedule_restart_first_extension_id_ = extension_id;
  } else if (extension_id != schedule_restart_first_extension_id_) {
    // We only allow the first extension to call this API to call it repeatedly.
    // Any other extension will fail.
    return RestartAfterDelayStatus::FAILED_NOT_FIRST_EXTENSION;
  }

  MaybeCancelRunningDelayedRestartTimer();

  if (seconds_from_now == -1) {
    // We already stopped the running timer (if any).
    return RestartAfterDelayStatus::SUCCESS_RESTART_CANCELED;
  }

  if (!did_read_delayed_restart_preferences_) {
    // Try to read any previous successful restart attempt time resulting from
    // this API.
    PrefService* pref_service =
        ExtensionsBrowserClient::Get()->GetPrefServiceForContext(
            browser_context_);
    DCHECK(pref_service);

    was_last_restart_due_to_delayed_restart_api_ =
        pref_service->GetBoolean(kPrefLastRestartWasDueToDelayedRestartApi);
    if (was_last_restart_due_to_delayed_restart_api_) {
      // We clear this bit if the previous restart was due to this API, so that
      // we don't throttle restart requests coming after other restarts or
      // shutdowns not caused by the runtime API.
      pref_service->SetBoolean(kPrefLastRestartWasDueToDelayedRestartApi,
                               false);
    }

    last_delayed_restart_time_ = base::Time::FromDoubleT(
        pref_service->GetDouble(kPrefLastRestartAfterDelayTime));

    if (!allow_non_kiosk_apps_restart_api_for_test) {
      // Don't read every time unless in tests.
      did_read_delayed_restart_preferences_ = true;
    }
  }

  return ScheduleDelayedRestart(now, seconds_from_now);
}

bool RuntimeAPI::OpenOptionsPage(const Extension* extension) {
  return delegate_->OpenOptionsPage(extension);
}

void RuntimeAPI::MaybeCancelRunningDelayedRestartTimer() {
  if (restart_after_delay_timer_.IsRunning())
    restart_after_delay_timer_.Stop();
}

RuntimeAPI::RestartAfterDelayStatus RuntimeAPI::ScheduleDelayedRestart(
    const base::Time& now,
    int seconds_from_now) {
  base::TimeDelta delay_till_restart =
      base::TimeDelta::FromSeconds(seconds_from_now);

  // Throttle restart requests that are received too soon successively, only if
  // the previous restart was due to this API.
  bool was_throttled = false;
  if (was_last_restart_due_to_delayed_restart_api_) {
    base::Time future_restart_time = now + delay_till_restart;
    base::TimeDelta delta_since_last_restart =
        future_restart_time > last_delayed_restart_time_
            ? future_restart_time - last_delayed_restart_time_
            : base::TimeDelta::Max();
    if (delta_since_last_restart < minimum_duration_between_restarts_) {
      // Schedule the restart after |minimum_duration_between_restarts_| has
      // passed.
      delay_till_restart = minimum_duration_between_restarts_ -
                           (now - last_delayed_restart_time_);
      was_throttled = true;
    }
  }

  restart_after_delay_timer_.Start(
      FROM_HERE, delay_till_restart,
      base::Bind(&RuntimeAPI::OnDelayedRestartTimerTimeout,
                 weak_ptr_factory_.GetWeakPtr()));

  return was_throttled ? RestartAfterDelayStatus::FAILED_THROTTLED
                       : RestartAfterDelayStatus::SUCCESS_RESTART_SCHEDULED;
}

void RuntimeAPI::OnDelayedRestartTimerTimeout() {
  // We can persist "now" as the last successful restart time, assuming that the
  // following restart request will succeed, since it can only fail if requested
  // by non kiosk apps, and we prevent that from the beginning (unless in
  // unit tests).
  // This assumption is important, since once restart is requested, we might not
  // have enough time to persist the data to disk.
  double now = base::Time::NowFromSystemTime().ToDoubleT();
  PrefService* pref_service =
      ExtensionsBrowserClient::Get()->GetPrefServiceForContext(
          browser_context_);
  DCHECK(pref_service);
  pref_service->SetDouble(kPrefLastRestartAfterDelayTime, now);
  pref_service->SetBoolean(kPrefLastRestartWasDueToDelayedRestartApi, true);

  std::string error_message;
  const bool success = delegate_->RestartDevice(&error_message);

  // Make sure our above assumption is maintained.
  DCHECK(success || allow_non_kiosk_apps_restart_api_for_test);
}

void RuntimeAPI::AllowNonKioskAppsInRestartAfterDelayForTesting() {
  allow_non_kiosk_apps_restart_api_for_test = true;
}

///////////////////////////////////////////////////////////////////////////////

// static
void RuntimeEventRouter::DispatchOnStartupEvent(
    content::BrowserContext* context,
    const std::string& extension_id) {
  DispatchOnStartupEventImpl(context, extension_id, true, NULL);
}

// static
void RuntimeEventRouter::DispatchOnInstalledEvent(
    content::BrowserContext* context,
    const std::string& extension_id,
    const Version& old_version,
    bool chrome_updated) {
  if (!ExtensionsBrowserClient::Get()->IsValidContext(context))
    return;
  ExtensionSystem* system = ExtensionSystem::Get(context);
  if (!system)
    return;

  std::unique_ptr<base::ListValue> event_args(new base::ListValue());
  base::DictionaryValue* info = new base::DictionaryValue();
  event_args->Append(info);
  if (old_version.IsValid()) {
    info->SetString(kInstallReason, kInstallReasonUpdate);
    info->SetString(kInstallPreviousVersion, old_version.GetString());
  } else if (chrome_updated) {
    info->SetString(kInstallReason, kInstallReasonChromeUpdate);
  } else {
    info->SetString(kInstallReason, kInstallReasonInstall);
  }
  EventRouter* event_router = EventRouter::Get(context);
  DCHECK(event_router);
  std::unique_ptr<Event> event(new Event(events::RUNTIME_ON_INSTALLED,
                                         runtime::OnInstalled::kEventName,
                                         std::move(event_args)));
  event_router->DispatchEventWithLazyListener(extension_id, std::move(event));

  if (old_version.IsValid()) {
    const Extension* extension =
        ExtensionRegistry::Get(context)->enabled_extensions().GetByID(
            extension_id);
    if (extension && SharedModuleInfo::IsSharedModule(extension)) {
      std::unique_ptr<ExtensionSet> dependents =
          system->GetDependentExtensions(extension);
      for (ExtensionSet::const_iterator i = dependents->begin();
           i != dependents->end();
           i++) {
        std::unique_ptr<base::ListValue> sm_event_args(new base::ListValue());
        base::DictionaryValue* sm_info = new base::DictionaryValue();
        sm_event_args->Append(sm_info);
        sm_info->SetString(kInstallReason, kInstallReasonSharedModuleUpdate);
        sm_info->SetString(kInstallPreviousVersion, old_version.GetString());
        sm_info->SetString(kInstallId, extension_id);
        std::unique_ptr<Event> sm_event(new Event(
            events::RUNTIME_ON_INSTALLED, runtime::OnInstalled::kEventName,
            std::move(sm_event_args)));
        event_router->DispatchEventWithLazyListener((*i)->id(),
                                                    std::move(sm_event));
      }
    }
  }
}

// static
void RuntimeEventRouter::DispatchOnUpdateAvailableEvent(
    content::BrowserContext* context,
    const std::string& extension_id,
    const base::DictionaryValue* manifest) {
  ExtensionSystem* system = ExtensionSystem::Get(context);
  if (!system)
    return;

  std::unique_ptr<base::ListValue> args(new base::ListValue);
  args->Append(manifest->DeepCopy());
  EventRouter* event_router = EventRouter::Get(context);
  DCHECK(event_router);
  std::unique_ptr<Event> event(new Event(events::RUNTIME_ON_UPDATE_AVAILABLE,
                                         runtime::OnUpdateAvailable::kEventName,
                                         std::move(args)));
  event_router->DispatchEventToExtension(extension_id, std::move(event));
}

// static
void RuntimeEventRouter::DispatchOnBrowserUpdateAvailableEvent(
    content::BrowserContext* context) {
  ExtensionSystem* system = ExtensionSystem::Get(context);
  if (!system)
    return;

  std::unique_ptr<base::ListValue> args(new base::ListValue);
  EventRouter* event_router = EventRouter::Get(context);
  DCHECK(event_router);
  std::unique_ptr<Event> event(new Event(
      events::RUNTIME_ON_BROWSER_UPDATE_AVAILABLE,
      runtime::OnBrowserUpdateAvailable::kEventName, std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

// static
void RuntimeEventRouter::DispatchOnRestartRequiredEvent(
    content::BrowserContext* context,
    const std::string& app_id,
    api::runtime::OnRestartRequiredReason reason) {
  ExtensionSystem* system = ExtensionSystem::Get(context);
  if (!system)
    return;

  std::unique_ptr<Event> event(
      new Event(events::RUNTIME_ON_RESTART_REQUIRED,
                runtime::OnRestartRequired::kEventName,
                api::runtime::OnRestartRequired::Create(reason)));
  EventRouter* event_router = EventRouter::Get(context);
  DCHECK(event_router);
  event_router->DispatchEventToExtension(app_id, std::move(event));
}

// static
void RuntimeEventRouter::OnExtensionUninstalled(
    content::BrowserContext* context,
    const std::string& extension_id,
    UninstallReason reason) {
  if (!(reason == UNINSTALL_REASON_USER_INITIATED ||
        reason == UNINSTALL_REASON_MANAGEMENT_API)) {
    return;
  }

  GURL uninstall_url(
      GetUninstallURL(ExtensionPrefs::Get(context), extension_id));

  if (!uninstall_url.SchemeIsHTTPOrHTTPS()) {
    // Previous versions of Chrome allowed non-http(s) URLs to be stored in the
    // prefs. Now they're disallowed, but the old data may still exist.
    return;
  }

  RuntimeAPI::GetFactoryInstance()->Get(context)->OpenURL(uninstall_url);
}

ExtensionFunction::ResponseAction RuntimeGetBackgroundPageFunction::Run() {
  ExtensionHost* host = ProcessManager::Get(browser_context())
                            ->GetBackgroundHostForExtension(extension_id());
  if (LazyBackgroundTaskQueue::Get(browser_context())
          ->ShouldEnqueueTask(browser_context(), extension())) {
    LazyBackgroundTaskQueue::Get(browser_context())
        ->AddPendingTask(
            browser_context(), extension_id(),
            base::Bind(&RuntimeGetBackgroundPageFunction::OnPageLoaded, this));
  } else if (host) {
    OnPageLoaded(host);
  } else {
    return RespondNow(Error(kNoBackgroundPageError));
  }

  return RespondLater();
}

void RuntimeGetBackgroundPageFunction::OnPageLoaded(ExtensionHost* host) {
  if (host) {
    Respond(NoArguments());
  } else {
    Respond(Error(kPageLoadError));
  }
}

ExtensionFunction::ResponseAction RuntimeOpenOptionsPageFunction::Run() {
  RuntimeAPI* api = RuntimeAPI::GetFactoryInstance()->Get(browser_context());
  return RespondNow(api->OpenOptionsPage(extension())
                        ? NoArguments()
                        : Error(kFailedToCreateOptionsPage));
}

ExtensionFunction::ResponseAction RuntimeSetUninstallURLFunction::Run() {
  std::string url_string;
  EXTENSION_FUNCTION_VALIDATE(args_->GetString(0, &url_string));

  if (!url_string.empty() && !GURL(url_string).SchemeIsHTTPOrHTTPS()) {
    return RespondNow(Error(kInvalidUrlError, url_string));
  }
  SetUninstallURL(
      ExtensionPrefs::Get(browser_context()), extension_id(), url_string);
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction RuntimeReloadFunction::Run() {
  RuntimeAPI::GetFactoryInstance()->Get(browser_context())->ReloadExtension(
      extension_id());
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction RuntimeRequestUpdateCheckFunction::Run() {
  if (!RuntimeAPI::GetFactoryInstance()
           ->Get(browser_context())
           ->CheckForUpdates(
               extension_id(),
               base::Bind(&RuntimeRequestUpdateCheckFunction::CheckComplete,
                          this))) {
    return RespondNow(Error(kUpdatesDisabledError));
  }
  return RespondLater();
}

void RuntimeRequestUpdateCheckFunction::CheckComplete(
    const RuntimeAPIDelegate::UpdateCheckResult& result) {
  if (result.success) {
    std::unique_ptr<base::DictionaryValue> details(new base::DictionaryValue);
    details->SetString("version", result.version);
    Respond(TwoArguments(base::MakeUnique<base::StringValue>(result.response),
                         std::move(details)));
  } else {
    // HMM(kalman): Why does !success not imply Error()?
    Respond(OneArgument(base::MakeUnique<base::StringValue>(result.response)));
  }
}

ExtensionFunction::ResponseAction RuntimeRestartFunction::Run() {
  std::string message;
  bool result =
      RuntimeAPI::GetFactoryInstance()->Get(browser_context())->RestartDevice(
          &message);
  if (!result) {
    return RespondNow(Error(message));
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction RuntimeRestartAfterDelayFunction::Run() {
  if (!allow_non_kiosk_apps_restart_api_for_test &&
      !ExtensionsBrowserClient::Get()->IsRunningInForcedAppMode()) {
    return RespondNow(Error(kErrorOnlyKioskModeAllowed));
  }

  std::unique_ptr<api::runtime::RestartAfterDelay::Params> params(
      api::runtime::RestartAfterDelay::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int seconds = params->seconds;

  if (seconds <= 0 && seconds != -1)
    return RespondNow(Error(kErrorInvalidArgument, base::IntToString(seconds)));

  RuntimeAPI::RestartAfterDelayStatus request_status =
      RuntimeAPI::GetFactoryInstance()
          ->Get(browser_context())
          ->RestartDeviceAfterDelay(extension()->id(), seconds);

  switch (request_status) {
    case RuntimeAPI::RestartAfterDelayStatus::FAILED_NOT_FIRST_EXTENSION:
      return RespondNow(Error(kErrorOnlyFirstExtensionAllowed));

    case RuntimeAPI::RestartAfterDelayStatus::FAILED_THROTTLED:
      return RespondNow(Error(kErrorRequestedTooSoon));

    case RuntimeAPI::RestartAfterDelayStatus::SUCCESS_RESTART_CANCELED:
    case RuntimeAPI::RestartAfterDelayStatus::SUCCESS_RESTART_SCHEDULED:
      return RespondNow(NoArguments());
  }

  NOTREACHED();
  return RespondNow(Error(kErrorInvalidStatus));
}

ExtensionFunction::ResponseAction RuntimeGetPlatformInfoFunction::Run() {
  runtime::PlatformInfo info;
  if (!RuntimeAPI::GetFactoryInstance()
           ->Get(browser_context())
           ->GetPlatformInfo(&info)) {
    return RespondNow(Error(kPlatformInfoUnavailable));
  }
  return RespondNow(
      ArgumentList(runtime::GetPlatformInfo::Results::Create(info)));
}

ExtensionFunction::ResponseAction
RuntimeGetPackageDirectoryEntryFunction::Run() {
  storage::IsolatedContext* isolated_context =
      storage::IsolatedContext::GetInstance();
  DCHECK(isolated_context);

  std::string relative_path = kPackageDirectoryPath;
  base::FilePath path = extension_->path();
  std::string filesystem_id = isolated_context->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal, std::string(), path, &relative_path);

  int renderer_id = render_frame_host()->GetProcess()->GetID();
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantReadFileSystem(renderer_id, filesystem_id);
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetString("fileSystemId", filesystem_id);
  dict->SetString("baseName", relative_path);
  return RespondNow(OneArgument(std::move(dict)));
}

}  // namespace extensions
