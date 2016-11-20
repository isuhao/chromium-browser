// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/file_type_policies_component_installer.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/version.h"
#include "chrome/common/safe_browsing/file_type_policies.h"
#include "components/component_updater/component_updater_paths.h"
#include "content/public/browser/browser_thread.h"

using component_updater::ComponentUpdateService;

namespace {
const base::FilePath::CharType kFileTypePoliciesBinaryPbFileName[] =
    FILE_PATH_LITERAL("download_file_types.pb");

// The SHA256 of the SubjectPublicKeyInfo used to sign the extension.
// The extension id is: khaoiebndkojlmppeemjhbpbandiljpe
const uint8_t kPublicKeySHA256[32] = {
    0xa7, 0x0e, 0x84, 0x1d, 0x3a, 0xe9, 0xbc, 0xff, 0x44, 0xc9, 0x71,
    0xf1, 0x0d, 0x38, 0xb9, 0xf4, 0x65, 0x92, 0x31, 0x01, 0x47, 0x3f,
    0x1b, 0x7c, 0x11, 0xb0, 0x85, 0x0f, 0xa3, 0xfd, 0xe1, 0xe5};

const char kFileTypePoliciesManifestName[] = "File Type Policies";

void LoadFileTypesFromDisk(const base::FilePath& pb_path) {
  if (pb_path.empty())
    return;

  VLOG(1) << "Reading Download File Types from file: " << pb_path.value();
  std::string binary_pb;
  if (!base::ReadFileToString(pb_path, &binary_pb)) {
    // The file won't exist on new installations, so this is not always an
    // error.
    VLOG(1) << "Failed reading from " << pb_path.value();
    return;
  }

  safe_browsing::FileTypePolicies::GetInstance()->PopulateFromDynamicUpdate(
      binary_pb);
}

}  // namespace

namespace component_updater {

bool FileTypePoliciesComponentInstallerTraits::CanAutoUpdate() const {
  return true;
}

bool FileTypePoliciesComponentInstallerTraits::RequiresNetworkEncryption()
    const {
  return false;
}

bool FileTypePoliciesComponentInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return true;  // Nothing custom here.
}

base::FilePath FileTypePoliciesComponentInstallerTraits::GetInstalledPath(
    const base::FilePath& base) {
  return base.Append(kFileTypePoliciesBinaryPbFileName);
}

void FileTypePoliciesComponentInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  VLOG(1) << "Component ready, version " << version.GetString() << " in "
          << install_dir.value();

  if (!content::BrowserThread::PostBlockingPoolTask(
          FROM_HERE,
          base::Bind(&LoadFileTypesFromDisk, GetInstalledPath(install_dir)))) {
    NOTREACHED();
  }
}

// Called during startup and installation before ComponentReady().
bool FileTypePoliciesComponentInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the proto here, since we'll do the checking
  // in PopulateFromDynamicUpdate().
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath FileTypePoliciesComponentInstallerTraits::GetRelativeInstallDir()
    const {
  return base::FilePath(FILE_PATH_LITERAL("FileTypePolicies"));
}

void FileTypePoliciesComponentInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string FileTypePoliciesComponentInstallerTraits::GetName() const {
  return kFileTypePoliciesManifestName;
}

update_client::InstallerAttributes
FileTypePoliciesComponentInstallerTraits::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void RegisterFileTypePoliciesComponent(ComponentUpdateService* cus,
                                       const base::FilePath& user_data_dir) {
  VLOG(1) << "Registering File Type Policies component.";

  std::unique_ptr<ComponentInstallerTraits> traits(
      new FileTypePoliciesComponentInstallerTraits());
  // |cus| will take ownership of |installer| during installer->Register(cus).
  DefaultComponentInstaller* installer =
      new DefaultComponentInstaller(std::move(traits));
  installer->Register(cus, base::Closure());
}

}  // namespace component_updater
