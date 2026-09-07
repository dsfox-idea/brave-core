 /* Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/notreached.h"

 namespace base {
 class FilePath;
 }  // namespace base

namespace {
// Growser-208: our own folder in the user's ~/Applications, not Brave's.
// These named "Brave Browser Apps.localized" and friends, so installing a web
// app created a Finder-visible folder in the user's Applications named after
// another product - and shared it with a real Brave install, both browsers
// writing shortcuts into one directory. "Growser Apps" is already the name on
// every other platform (IDS_APP_SHORTCUTS_SUBDIR_NAME).
//
// Nothing is moved: shortcuts already sitting in the old folder stay there,
// because that folder may be Brave's and is not ours to tidy. New ones are
// written here, and Chromium rewrites a web app's shortcut when the app is
// updated or reinstalled.
base::FilePath GetLocalizableBraveAppShortcutsSubdirName();
}

#define BRAVE_GET_CHROME_APPS_FOLDER_IMPL \
  return path.Append(GetLocalizableBraveAppShortcutsSubdirName());

#include <chrome/browser/web_applications/os_integration/mac/apps_folder_support.mm>
#undef BRAVE_GET_CHROME_APPS_FOLDER_IMPL

namespace {
constexpr char kBraveBrowserDevelopmentAppDirName[] =
    "Growser Development Apps.localized";
constexpr char kBraveBrowserAppDirName[] = "Growser Apps.localized";
constexpr char kBraveBrowserBetaAppDirName[] =
    "Growser Beta Apps.localized";
constexpr char kBraveBrowserDevAppDirName[] =
    "Growser Dev Apps.localized";
constexpr char kBraveBrowserNightlyAppDirName[] =
    "Growser Nightly Apps.localized";

base::FilePath GetLocalizableBraveAppShortcutsSubdirName() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      return base::FilePath(kBraveBrowserAppDirName);
    case version_info::Channel::BETA:
      return base::FilePath(kBraveBrowserBetaAppDirName);
    case version_info::Channel::DEV:
      return base::FilePath(kBraveBrowserDevAppDirName);
    case version_info::Channel::CANARY:
      return base::FilePath(kBraveBrowserNightlyAppDirName);
    case version_info::Channel::UNKNOWN:
      return base::FilePath(kBraveBrowserDevelopmentAppDirName);
  }

  NOTREACHED() << "All possible channels are handled above.";
}
}  // namespace
