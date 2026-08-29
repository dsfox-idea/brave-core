/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/warm_new_tab_page_manager.h"

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// The warm NTP is heavily integration-bound (it creates a WebContents that
// loads chrome://newtab and its own ntp_tiles::MostVisitedSites, which needs a
// full profile). Its warm-build, [+] adoption and top-sites invalidation are
// exercised at the browser level and were verified at runtime; these unit tests
// cover the parts that stand on their own - the kill switch and its effect on
// the adoption entry point.
namespace growser {

TEST(WarmNewTabPageTest, FeatureEnabledByDefault) {
  EXPECT_TRUE(base::FeatureList::IsEnabled(kWarmNewTabPage));
}

// With the feature off, the adoption entry point returns null before it ever
// touches the browser, so a disabled build opens tabs the ordinary way.
TEST(WarmNewTabPageTest, AdoptionInertWhenDisabled) {
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(kWarmNewTabPage);
  EXPECT_EQ(MaybeAdoptWarmNewTab(nullptr), nullptr);
}

}  // namespace growser
