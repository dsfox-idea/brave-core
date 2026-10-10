/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/version_info/nix/version_extra_utils.h"

#include <string>
#include <string_view>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/scoped_environment_variable_override.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace version_info::nix {

namespace {

// RDN as the package declares it - the name of the .desktop file the portal
// looks our app id up in.
std::string RdnFromPackageInfo() {
  const base::FilePath info =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave/chromium_src/chrome/installer/linux/common/"
                       "brave-browser/chromium-browser.info");
  std::string text;
  EXPECT_TRUE(base::ReadFileToString(info, &text)) << info;
  for (std::string_view line : base::SplitStringPiece(
           text, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (auto value = base::RemovePrefix(line, "RDN=")) {
      return std::string(base::TrimString(*value, "\"", base::TRIM_ALL));
    }
  }
  ADD_FAILURE() << "no RDN= line in " << info;
  return std::string();
}

std::string AppNameOnChannel(const char* channel) {
  base::ScopedEnvironmentVariableOverride extra(kChromeVersionExtra, channel);
  return GetAppName(*extra.GetEnv());
}

}  // namespace

TEST(GrowserAppNameTest, IsOursAndMatchesThePackage) {
  EXPECT_EQ("org.growser.Browser", AppNameOnChannel("stable"));
  EXPECT_EQ(RdnFromPackageInfo(), AppNameOnChannel("stable"));
}

// installer.py names the hidden .desktop file RDN.<channel> off stable, with
// these channel names, so the browser must append the same suffixes.
TEST(GrowserAppNameTest, KeepsTheInstallerChannelSuffix) {
  EXPECT_EQ("org.growser.Browser.beta", AppNameOnChannel("beta"));
  EXPECT_EQ("org.growser.Browser.unstable", AppNameOnChannel("unstable"));
  EXPECT_EQ("org.growser.Browser.canary", AppNameOnChannel("canary"));
}

}  // namespace version_info::nix
