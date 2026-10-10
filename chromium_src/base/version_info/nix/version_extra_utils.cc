/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/version_info/nix/version_extra_utils.h"

#include <string_view>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"

#define GetAppName GetAppName_ChromiumImpl
#include <base/version_info/nix/version_extra_utils.cc>
#undef GetAppName

namespace version_info::nix {

// The XDG desktop portal and the systemd scope (app-<id>-<pid>.scope) know the
// browser by this id, and Chromium's value is the one a real Chromium install
// registers with - so the two browsers would share every portal grant. It must
// equal RDN in chromium_src/chrome/installer/linux/common/brave-browser/
// chromium-browser.info, which names the .desktop file the portal reads.
// Chromium's channel suffix is kept: the installer appends the same one.
std::string GetAppName(base::Environment& env) {
  static constexpr std::string_view kChromiumAppName = "org.chromium.Chromium";
  static constexpr std::string_view kGrowserAppName = "org.growser.Browser";
  const std::string chromium = GetAppName_ChromiumImpl(env);
  return base::StrCat(
      {kGrowserAppName,
       base::RemovePrefix(chromium, kChromiumAppName).value_or("")});
}

}  // namespace version_info::nix
