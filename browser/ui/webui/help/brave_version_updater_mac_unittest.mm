/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <limits.h>

#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/updater/buildflags.h"  // Growser-263
#include "chrome/browser/ui/webui/help/version_updater.h"
#include "chrome/browser/ui/webui/help/version_updater_mac.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(ENABLE_OMAHA4)  // Growser-263
#include "brave/browser/updater/features.h"
#endif

class BraveVersionUpdaterMacTest : public testing::Test {
 protected:
  bool UsesSparkle() {
    std::unique_ptr<VersionUpdater> updater = VersionUpdater::Create(nullptr);
    bool result = false;
    updater->GetIsSparkleForTesting(result);
    return result;
  }
  base::test::ScopedFeatureList scoped_feature_list_;
};

#if BUILDFLAG(ENABLE_OMAHA4)  // Growser-263
TEST_F(BraveVersionUpdaterMacTest, UsesSparkleWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(brave_updater::kBraveUseOmaha4);
  EXPECT_TRUE(UsesSparkle());
}

TEST_F(BraveVersionUpdaterMacTest, UsesOmaha4WhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      brave_updater::kBraveUseOmaha4,
      {{brave_updater::kLegacyFallbackIntervalDays.name,
        base::NumberToString(INT_MAX)}});
  EXPECT_FALSE(UsesSparkle());
}
#else   // Growser-263
// With no Omaha built, the About page asks Sparkle. It used to get upstream's
// updater instead, which has no process to reach and showed every user "error
// 9 (error code 0)" in red while the updates themselves worked.
TEST_F(BraveVersionUpdaterMacTest, UsesSparkleWithoutOmaha4) {
  EXPECT_TRUE(UsesSparkle());
}
#endif  // BUILDFLAG(ENABLE_OMAHA4)
