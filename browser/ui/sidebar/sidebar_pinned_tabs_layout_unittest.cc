/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_pinned_tabs_layout.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace sidebar {

namespace {

// The real sidebar numbers: SidebarButtonView::kSidebarButtonSize and
// ::kMargin, with a one pixel separator above the block.
constexpr int kEntry = 32;
constexpr int kSpacing = 4;
constexpr int kSeparator = 1;

int Capacity(int available_height) {
  return CalculatePinnedTabsCapacity(available_height, kEntry, kSpacing,
                                     kSeparator);
}

}  // namespace

TEST(SidebarPinnedTabsLayoutTest, NoRoomHostsNothing) {
  EXPECT_EQ(0, Capacity(0));
  EXPECT_EQ(0, Capacity(-100));
  // One pixel short of the first entry.
  EXPECT_EQ(0, Capacity(kSeparator + kSpacing + kEntry + kSpacing - 1));
}

TEST(SidebarPinnedTabsLayoutTest, ExactFit) {
  // separator + spacing + entry + spacing.
  EXPECT_EQ(1, Capacity(kSeparator + kSpacing + kEntry + kSpacing));
  EXPECT_EQ(2, Capacity(kSeparator + kSpacing + 2 * (kEntry + kSpacing)));
  EXPECT_EQ(10, Capacity(kSeparator + kSpacing + 10 * (kEntry + kSpacing)));
}

TEST(SidebarPinnedTabsLayoutTest, PartialEntryDoesNotCount) {
  // Room for three and most of a fourth: the fourth stays on the tab strip
  // rather than being squeezed in.
  const int three = kSeparator + kSpacing + 3 * (kEntry + kSpacing);
  EXPECT_EQ(3, Capacity(three));
  EXPECT_EQ(3, Capacity(three + kEntry));
  EXPECT_EQ(4, Capacity(three + kEntry + kSpacing));
}

TEST(SidebarPinnedTabsLayoutTest, GrowsAndShrinksWithHeight) {
  // The number only ever moves with the height, so a window resize can move
  // entries in either direction without any other state.
  int previous = 0;
  for (int height = 0; height < 1000; ++height) {
    const int capacity = Capacity(height);
    EXPECT_GE(capacity, previous);
    EXPECT_LE(capacity - previous, 1);
    previous = capacity;
  }
}

TEST(SidebarPinnedTabsLayoutTest, DegenerateEntryHeight) {
  EXPECT_EQ(0, CalculatePinnedTabsCapacity(1000, 0, kSpacing, kSeparator));
  EXPECT_EQ(0, CalculatePinnedTabsCapacity(1000, -1, kSpacing, kSeparator));
}

}  // namespace sidebar
