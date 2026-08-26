/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/tile_icon_fetcher.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_new_tab {

TEST(TileIconFetcherTest, AsksTheSiteRootWhateverPageIsOnTheBoard) {
  EXPECT_EQ(GURL("https://example.com/apple-touch-icon.png"),
            TouchIconURLFor(GURL("https://example.com/deep/page?a=1#top")));
  EXPECT_EQ(GURL("http://example.com/apple-touch-icon.png"),
            TouchIconURLFor(GURL("http://example.com/")));
}

TEST(TileIconFetcherTest, KeepsThePortAndTheSubdomain) {
  // A tile can point at either, and the icon belongs to that exact origin.
  EXPECT_EQ(GURL("https://docs.example.com:8443/apple-touch-icon.png"),
            TouchIconURLFor(GURL("https://docs.example.com:8443/a")));
}

TEST(TileIconFetcherTest, RefusesWhatIsNotAWebPage) {
  EXPECT_FALSE(TouchIconURLFor(GURL("file:///c:/notes.txt")).is_valid());
  EXPECT_FALSE(TouchIconURLFor(GURL("chrome://settings")).is_valid());
  EXPECT_FALSE(TouchIconURLFor(GURL("not a url")).is_valid());
}

TEST(TileIconFetcherTest, WaitsOutTheRetryInterval) {
  const base::Time now = base::Time::Now();
  EXPECT_FALSE(AttemptHasLapsed(now, now));
  EXPECT_FALSE(AttemptHasLapsed(now - base::Days(20), now));
  EXPECT_FALSE(
      AttemptHasLapsed(now - TileIconFetcher::kRetryInterval + base::Hours(1),
                       now));
  EXPECT_TRUE(AttemptHasLapsed(now - TileIconFetcher::kRetryInterval, now));
  EXPECT_TRUE(AttemptHasLapsed(now - base::Days(60), now));
}

TEST(TileIconFetcherTest, TreatsAClockThatWentBackwardsAsLapsed) {
  // Otherwise an attempt stamped in the future would pin a site to "asked
  // recently" until the clock caught up with it.
  const base::Time now = base::Time::Now();
  EXPECT_TRUE(AttemptHasLapsed(now + base::Days(400), now));
}

TEST(TileIconFetcherTest, WantsAnIconBigEnoughForATwoTimesDisplay) {
  // A tile draws its icon at 56 CSS pixels, which is 112 real ones at 2x.
  EXPECT_EQ(112, TileIconFetcher::kWantedIconSize);
  EXPECT_EQ(base::Days(21), TileIconFetcher::kRetryInterval);
}

}  // namespace brave_new_tab
