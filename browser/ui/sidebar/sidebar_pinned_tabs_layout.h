/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_PINNED_TABS_LAYOUT_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_PINNED_TABS_LAYOUT_H_

#include "third_party/skia/include/core/SkColor.h"

namespace sidebar {

// How many pinned tabs the sidebar can host in |available_height|.
//
// Entries are laid out like every other sidebar button: a fixed |entry_height|
// each, with |spacing| above the first, between neighbours and below the last
// (margin collapsing makes the gap between two entries one |spacing|, not two).
// Pinned entries never shrink to let one more in - that is how the tab strip
// treats pinned tabs, where minimum, crossover and preferred width all collapse
// to the pinned width. Whatever does not fit stays on the tab strip.
//
// |leading_height| is what the block spends before its first entry (the
// separator that divides it from the built-in sidebar items).
int CalculatePinnedTabsCapacity(int available_height,
                                int entry_height,
                                int spacing,
                                int leading_height);

// Growser-183: whether the active entry's glow belongs on a sidebar of this
// colour. The glow reads as light coming off the edge on a dark surface; on a
// light one the same shadow only tints the background around the bar, which
// the bar alone says better. Asked of the surface's colour rather than of the
// OS setting, so a dark custom theme under a light system still gets it.
bool ShouldPaintActiveTabGlow(SkColor surface_color);

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_PINNED_TABS_LAYOUT_H_
