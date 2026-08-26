/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "base/check_op.h"
#include "base/dcheck_is_on.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/setup/brand_behaviors.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/install_util.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace installer {

namespace {

bool NavigateToUrlWithHttps(const std::wstring& url);

}  // namespace

// If |archive_type| is INCREMENTAL_ARCHIVE_TYPE and |install_status| does not
// indicate a successful update, "-full" is appended to Chrome's "ap" value in
// its ClientState key if it is not present, resulting in the full installer
// being returned from the next update check. If |archive_type| is
// FULL_ARCHIVE_TYPE or |install_status| indicates a successful update, "-full"
// is removed from the "ap" value. "-stage:*" values are
// unconditionally removed from the "ap" value.
// This function used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
void UpdateInstallStatus(installer::ArchiveType archive_type,
                         installer::InstallStatus install_status) {
  GoogleUpdateSettings::UpdateInstallStatus(
      install_static::IsSystemInstall(), archive_type,
      InstallUtil::GetInstallReturnCode(install_status));
}

void DoPostUninstallOperations(const base::Version& /*version*/,
                               const base::FilePath& /*local_data_path*/,
                               const std::wstring& /*distribution_data*/) {
  // growser (#78): uninstalling opens nothing.
  //
  // What it used to open was brave.com/uninstall-survey, with our version and
  // the user's Windows build appended - so the last thing our browser did on
  // the way out was hand someone to Brave and tell them a "Brave" install had
  // been removed. We have no survey of our own, and the honest alternative to
  // somebody else's is none.
  //
  // Verified in the artifact rather than in the source: the string was in
  // setup.exe (`uninstall-survey`, once), which is what actually runs at
  // uninstall time.
}

class GoogleUpdateSettings_UNUSED {
 public:
  static void UpdateInstallStatus() { NOTREACHED(); }
};

}  // namespace installer

#define GoogleUpdateSettings GoogleUpdateSettings_UNUSED
#define DoPostUninstallOperations DoPostUninstallOperations_UNUSED
#include <chrome/installer/setup/google_chrome_behaviors.cc>
#undef DoPostUninstallOperations
#undef GoogleUpdateSettings
