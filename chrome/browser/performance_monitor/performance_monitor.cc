// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/performance_monitor/performance_monitor.h"

#include <stddef.h>
#include <utility>

#include "base/memory/singleton.h"
#include "base/process/process_iterator.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/browser_child_process_host_iterator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_constants.h"

#if defined(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/manifest_handlers/background_info.h"
#endif

using content::BrowserThread;

namespace {

// The default interval at which PerformanceMonitor performs its timed
// collections.
const int kGatherIntervalInSeconds = 120;

}  // namespace

namespace performance_monitor {

PerformanceMonitor::PerformanceMonitor() {
}

PerformanceMonitor::~PerformanceMonitor() {
}

// static
PerformanceMonitor* PerformanceMonitor::GetInstance() {
  return base::Singleton<PerformanceMonitor>::get();
}

void PerformanceMonitor::StartGatherCycle() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  repeating_timer_.Start(FROM_HERE,
                         base::TimeDelta::FromSeconds(kGatherIntervalInSeconds),
                         this, &PerformanceMonitor::GatherMetricsMapOnUIThread);
}

namespace {

void GatherMetricsForRenderProcess(content::RenderProcessHost* host,
                                   ProcessMetricsMetadata& data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
#if defined(ENABLE_EXTENSIONS)
  content::BrowserContext* browser_context = host->GetBrowserContext();
  extensions::ProcessMap* extension_process_map =
      extensions::ProcessMap::Get(browser_context);

  std::set<std::string> extension_ids =
      extension_process_map->GetExtensionsInProcess(host->GetID());

  // We only collect more granular metrics when there's only one extension
  // running in a given renderer, to reduce noise.
  if (extension_ids.size() != 1)
    return;

  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistry::Get(browser_context);

  const extensions::Extension* extension =
      extension_registry->enabled_extensions().GetByID(*extension_ids.begin());

  if (!extension)
    return;

  if (extensions::BackgroundInfo::HasPersistentBackgroundPage(extension)) {
    data.process_subtype = kProcessSubtypeExtensionPersistent;
  } else {
    data.process_subtype = kProcessSubtypeExtensionEvent;
  }
#endif
}

}  // namespace

void PerformanceMonitor::GatherMetricsMapOnUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  static int current_update_sequence = 0;
  // Even in the "somewhat" unlikely event this wraps around,
  // it doesn't matter. We just check it for inequality.
  current_update_sequence++;

  // Find all render child processes; has to be done on the UI thread.
  for (content::RenderProcessHost::iterator rph_iter =
           content::RenderProcessHost::AllHostsIterator();
       !rph_iter.IsAtEnd(); rph_iter.Advance()) {
    content::RenderProcessHost* host = rph_iter.GetCurrentValue();
    ProcessMetricsMetadata data;
    data.process_type = content::PROCESS_TYPE_RENDERER;
    data.handle = host->GetHandle();

    GatherMetricsForRenderProcess(host, data);
    MarkProcessAsAlive(data, current_update_sequence);
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&PerformanceMonitor::GatherMetricsMapOnIOThread,
                 base::Unretained(this), current_update_sequence));
}

void PerformanceMonitor::MarkProcessAsAlive(
    const ProcessMetricsMetadata& process_data,
    int current_update_sequence) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const base::ProcessHandle& handle = process_data.handle;
  if (handle == base::kNullProcessHandle) {
    // Process may not be valid yet.
    return;
  }

  MetricsMap::iterator process_metrics_iter = metrics_map_.find(handle);
  if (process_metrics_iter == metrics_map_.end()) {
    // If we're not already watching the process, let's initialize it.
    metrics_map_[handle].Initialize(process_data, current_update_sequence);
  } else {
    // If we are watching the process, touch it to keep it alive.
    ProcessMetricsHistory& process_metrics = process_metrics_iter->second;
    process_metrics.set_last_update_sequence(current_update_sequence);
  }
}

void PerformanceMonitor::GatherMetricsMapOnIOThread(
    int current_update_sequence) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<std::vector<ProcessMetricsMetadata>> process_data_list(
      new std::vector<ProcessMetricsMetadata>());

  // Find all child processes (does not include renderers), which has to be
  // done on the IO thread.
  for (content::BrowserChildProcessHostIterator iter; !iter.Done(); ++iter) {
    ProcessMetricsMetadata child_process_data;
    child_process_data.handle = iter.GetData().handle;
    child_process_data.process_type = iter.GetData().process_type;

    if (iter.GetData().name == base::ASCIIToUTF16(content::kFlashPluginName)) {
      child_process_data.process_subtype = kProcessSubtypePPAPIFlash;
    }

    process_data_list->push_back(child_process_data);
  }

  // Add the current (browser) process.
  ProcessMetricsMetadata browser_process_data;
  browser_process_data.process_type = content::PROCESS_TYPE_BROWSER;
  browser_process_data.handle = base::GetCurrentProcessHandle();
  process_data_list->push_back(browser_process_data);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PerformanceMonitor::MarkProcessesAsAliveOnUIThread,
                 base::Unretained(this),
                 base::Passed(std::move(process_data_list)),
                 current_update_sequence));
}

void PerformanceMonitor::MarkProcessesAsAliveOnUIThread(
    std::unique_ptr<std::vector<ProcessMetricsMetadata>> process_data_list,
    int current_update_sequence) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (size_t i = 0; i < process_data_list->size(); ++i) {
    MarkProcessAsAlive((*process_data_list)[i], current_update_sequence);
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&PerformanceMonitor::UpdateMetricsOnIOThread,
                 base::Unretained(this), current_update_sequence));
}

void PerformanceMonitor::UpdateMetricsOnIOThread(int current_update_sequence) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Update metrics for all watched processes; remove dead entries from the map.
  MetricsMap::iterator iter = metrics_map_.begin();
  while (iter != metrics_map_.end()) {
    ProcessMetricsHistory& process_metrics = iter->second;
    if (process_metrics.last_update_sequence() != current_update_sequence) {
      // Not touched this iteration; let's get rid of it.
      metrics_map_.erase(iter++);
    } else {
      process_metrics.SampleMetrics();
      ++iter;
    }
  }

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&PerformanceMonitor::RunTriggersUIThread,
                                     base::Unretained(this)));
}

void PerformanceMonitor::RunTriggersUIThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (auto it = metrics_map_.begin(); it != metrics_map_.end(); ++it) {
    it->second.RunPerformanceTriggers();
  }

  StartGatherCycle();
}

}  // namespace performance_monitor
