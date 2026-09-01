/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"

#include "base/check.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

// Growser-140
// The leading comma runs the sidebar pass before either branch computes the
// bounds; the value the call site receives is still the ternary's.
#define CalculateTabBounds                                          \
  HidePinnedTabsHostedBySidebar(tab_widths),                        \
      use_vertical_tabs_&& FillGroupInfo(tab_widths) &&             \
              FillNestingInfo(tab_widths)                           \
          ? tabs::CalculateVerticalTabBounds(                       \
                tab_widths, available_width,                        \
                GetBraveTabStrip() -> ShouldShowPinnedTabsInGrid()) \
          : CalculateTabBounds

#include <chrome/browser/ui/views/tabs/tab_strip_layout_helper.cc>

#undef CalculateTabBounds

BraveTabStrip* TabStripLayoutHelper::GetBraveTabStrip() const {
  return static_cast<BraveTabStrip*>(tab_strip_.get());
}

// Unfortunately, TabStripLayout::TabSlot is declared and defined in the .cc
// file, we can't move this method out of this file.
bool TabStripLayoutHelper::FillGroupInfo(
    std::vector<TabWidthConstraints>& tab_widths) {
  DCHECK(use_vertical_tabs_)
      << "Must be called only when |use_vertical_tabs_| is true";

  for (auto i = 0u; i < tab_widths.size(); i++) {
    auto& tab_width_constraints = tab_widths.at(i);
    tab_width_constraints.set_is_tab_in_group(
        slots_.at(i).type == TabSlotView::ViewType::kTab &&
        slots_.at(i).view->group().has_value());
  }
  return true;
}

// Growser-140
// A pinned tab the sidebar is showing must not be drawn in the strip as well.
// Marking it closed is how a tab takes no room here - collapsed tree tabs do
// the same below - and it leaves the pinned tabs that stay behind at their
// fixed width rather than squeezing them.
void TabStripLayoutHelper::HidePinnedTabsHostedBySidebar(
    std::vector<TabWidthConstraints>& tab_widths) {
  // A vertical strip shows pinned tabs in a column of its own already.
  if (use_vertical_tabs_) {
    return;
  }

  auto* brave_tab_strip = GetBraveTabStrip();
  if (!brave_tab_strip) {
    return;
  }

  int remaining = brave_tab_strip->GetPinnedTabCountHostedBySidebar();
  for (size_t i = 0; i < tab_widths.size() && remaining > 0; i++) {
    if (slots_.at(i).type != TabSlotView::ViewType::kTab ||
        tab_widths[i].state().pinned() != TabPinned::kPinned) {
      continue;
    }

    tab_widths[i].state().set_open(TabOpen::kClosed);
    remaining--;
  }
}

bool TabStripLayoutHelper::FillNestingInfo(
    std::vector<TabWidthConstraints>& tab_widths) {
  if (!use_vertical_tabs_ || !use_tree_tabs_) {
    return true;
  }

  for (int i = 0; i < static_cast<int>(slots_.size()); i++) {
    auto& tab_width = tab_widths[i];
    auto& slot_view = slots_.at(i).view;
    tab_width.state().set_nesting_info(slot_view->GetTabNestingInfo());
    if (slot_view->IsInCollapsedTreeTabNode()) {
      tab_width.state().set_open(TabOpen::kClosed);
    }
  }
  return true;
}
