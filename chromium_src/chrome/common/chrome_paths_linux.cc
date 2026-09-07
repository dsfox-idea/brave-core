/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/nix/xdg_util.h"
#include "brave/common/brave_channel_info_posix.h"
#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/common/chrome_paths_internal.h"

namespace {

// Brave doesn't use CHROME_CONFIG_HOME or Google Chrome's directory names; it
// always lives under BraveSoftware, with a channel-specific suffix and a
// separate name when Brave Origin branded.
//
// Growser-206: except that it must not be BraveSoftware for us. This one
// string decides the whole user data directory on Linux - profile, cache,
// extensions, and the Crashpad database, which does NOT follow
// --user-data-dir and so lands here whatever a test browser is told.
//
// It survived the entire rebrand because nothing checks it: every string a
// person reads says Growser, the About page says Growser, the bundle id is
// ours - and this, which only the system reads, still said Brave. On a
// machine with the real Brave installed that is not a blemish, it is two
// browsers sharing one profile: one cookie jar, one extension set, one crash
// database, belonging to whichever ran last. Measured on macOS, where the
// same string is CrProductDirName and the owner has Brave installed.
//
// ~/.config/growser, flat and lower case, is the Linux idiom - Chrome uses
// google-chrome and Chromium uses chromium - and it matches the binary this
// package installs.
bool BraveGetDefaultUserDataDirectory(base::FilePath* result) {
  auto env = base::Environment::Create();
  base::FilePath config_dir = base::nix::GetXDGDirectory(
      env.get(), base::nix::kXdgConfigHomeEnvVar, base::nix::kDotConfigDir);

  std::string data_dir_suffix;
  brave::GetChannelImpl(nullptr, &data_dir_suffix);

#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  *result = config_dir.Append("growser-origin" + data_dir_suffix);
#else
  *result = config_dir.Append("growser" + data_dir_suffix);
#endif
  return true;
}

}  // namespace

#include <chrome/common/chrome_paths_linux.cc>
