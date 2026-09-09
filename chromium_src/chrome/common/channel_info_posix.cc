/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/channel_info.h"

#include <string_view>

#include "base/environment.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/common/brave_channel_info_posix.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "build/build_config.h"
#include "components/version_info/version_info.h"

namespace chrome {

std::string GetChannelName(WithExtendedStable with_extended_stable) {
  std::string modifier;
  brave::GetChannelImpl(&modifier, nullptr);
  return modifier;
}

std::string GetChannelSuffixForDataDir() {
  return std::string();
}

#if BUILDFLAG(IS_LINUX)
std::string GetChannelSuffixForExtraFlagsEnvVarName() {
#if defined(OFFICIAL_BUILD)
  version_info::Channel product_channel(chrome::GetChannel());
  switch (product_channel) {
    case version_info::Channel::DEV:
      return "_DEV";
    case version_info::Channel::BETA:
      return "_BETA";
    case version_info::Channel::CANARY:
      return "_NIGHTLY";
    case version_info::Channel::STABLE:
      return "_STABLE";
    default:
      return std::string();
  }
#else   // defined(OFFICIAL_BUILD)
  const char* const channel_name = getenv("CHROME_VERSION_EXTRA");
  return channel_name
             ? base::StrCat(
                   {"_", base::ToUpperASCII(std::string_view(channel_name))})
             : std::string();
#endif  // defined(OFFICIAL_BUILD)
}
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_LINUX)
std::string GetDesktopName(base::Environment* env) {
  // Growser-225: the name here is handed to `xdg-settings set
  // default-web-browser`, so it has to be a file that exists on the machine.
  // It said brave.desktop under a snap and brave-browser.desktop otherwise,
  // and neither is installed by anything we ship - which reads as a browser
  // that merely is not the default, and so went unnoticed.
  if (auto growser_snap = env->GetVar("GROWSER_SNAP");
      growser_snap && *growser_snap == "1") {
    // snapd names the file it publishes <instance>_<app>.desktop, even when
    // the two are the same word. Measured on an installed snap: the only file
    // in /var/lib/snapd/desktop/applications is growser_growser.desktop, and
    // growser.desktop is not there. Brave's value here was "brave.desktop",
    // which by the same rule is a file their snap does not install either -
    // this is not a translation of it.
    return "growser_growser.desktop";
  }
#if defined(OFFICIAL_BUILD)
  version_info::Channel product_channel(chrome::GetChannel());
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  switch (product_channel) {
    case version_info::Channel::DEV:
      return "brave-origin-dev.desktop";
    case version_info::Channel::BETA:
      return "brave-origin-beta.desktop";
    case version_info::Channel::CANARY:
      return "brave-origin-nightly.desktop";
    default:
      return "brave-origin.desktop";
  }
#else   // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  switch (product_channel) {
    case version_info::Channel::DEV:
      return "growser-dev.desktop";
    case version_info::Channel::BETA:
      return "growser-beta.desktop";
    case version_info::Channel::CANARY:
      return "growser-nightly.desktop";
    default:
      return "growser.desktop";
  }
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#else   // defined(OFFICIAL_BUILD)
  // Allow $CHROME_DESKTOP to override the built-in value, so that development
  // versions can set themselves as the default without interfering with
  // non-official, packaged versions using the built-in value.
  if (std::string name = env->GetVar("CHROME_DESKTOP").value_or(std::string());
      !name.empty()) {
    return name;
  }
  return "growser.desktop";
#endif
}
#endif  // BUILDFLAG(IS_LINUX)

version_info::Channel GetChannel() {
  return brave::GetChannelImpl(nullptr, nullptr);
}

bool IsExtendedStableChannel() {
  // No extended stable channel for Brave.
  return false;
}

}  // namespace chrome
