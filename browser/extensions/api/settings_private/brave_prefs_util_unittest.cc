/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/settings_private/brave_prefs_util.h"

#include "brave/components/sidebar/browser/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

// A settings row binds a control to a pref by name, and settingsPrivate serves
// only the prefs on this list. A row bound to a pref that is missing here reads
// as "off" whatever the pref says and does nothing when clicked - with no error
// anywhere. That shipped once (growser#140, caught by looking at the page), so
// the sidebar's rows are pinned here.
class BravePrefsUtilTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(BravePrefsUtilTest, SidebarSettingsRowsAreReachableFromSettings) {
  BravePrefsUtil prefs_util(&profile_);
  const auto& allowlist = prefs_util.GetAllowlistedKeys();

  EXPECT_TRUE(allowlist.contains(sidebar::kSidebarShowOption));
  EXPECT_TRUE(allowlist.contains(sidebar::kSidebarShowPinnedTabs));
}

}  // namespace extensions
