/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TABS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TABS_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

class BraveBrowser;
class SidebarItemView;

namespace views {
class Separator;
}  // namespace views

// The pinned tabs the sidebar hosts, drawn below the built-in sidebar items and
// divided from them by a separator - the same divider the tab strip draws
// between pinned and unpinned tabs.
//
// The block hosts as many pinned tabs as fit by height and no more: entries
// keep a fixed height and are never squeezed to let one more in. Whatever does
// not fit stays on the tab strip, and the count is republished on every layout,
// so resizing the window moves entries between the two surfaces on its own.
//
// It only ever hosts anything when the sidebar is permanently visible and the
// tab strip is horizontal - see IsHostingEnabled().
class SidebarPinnedTabsView : public views::View, public TabStripModelObserver {
  METADATA_HEADER(SidebarPinnedTabsView, views::View)

 public:
  explicit SidebarPinnedTabsView(BraveBrowser* browser);
  ~SidebarPinnedTabsView() override;

  SidebarPinnedTabsView(const SidebarPinnedTabsView&) = delete;
  SidebarPinnedTabsView& operator=(const SidebarPinnedTabsView&) = delete;

  // How many pinned tabs this view is showing right now. The tab strip skips
  // exactly this many leading pinned tabs.
  int hosted_count() const { return hosted_count_; }

  // views::View:
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  void OnThemeChanged() override;
  void VisibilityChanged(views::View* starting_from, bool is_visible) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void OnTabPinnedStateChanged(tabs::TabInterface* tab, int index) override;
  void OnTabChangedAt(tabs::TabInterface* tab,
                      int index,
                      TabChangeType change_type) override;

 private:
  // False when the feature is off, the tab strip is vertical, or the sidebar is
  // not permanently visible. In the last case hosting would hide pinned tabs
  // along with the sidebar, or make them jump surfaces on every hover.
  bool IsHostingEnabled() const;

  // True once the window is closing every tab it has.
  bool IsWindowClosing() const;

  // Recreates one entry per pinned tab. Cheap: pinned tabs are few, and this
  // only runs when the pinned set itself changes.
  void RebuildEntries();
  void UpdateEntry(size_t entry_index);
  void OnEntryPressed(size_t entry_index);
  void OnSettingChanged();

  // Tells the tab strip how many leading pinned tabs it must not draw.
  void PublishHostedCount(int count);

  raw_ptr<BraveBrowser> browser_ = nullptr;
  raw_ptr<views::Separator> separator_ = nullptr;
  std::vector<raw_ptr<SidebarItemView>> entries_;
  int hosted_count_ = 0;

  BooleanPrefMember show_pinned_tabs_;
  BooleanPrefMember vertical_tabs_enabled_;
  IntegerPrefMember sidebar_show_option_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TABS_VIEW_H_
