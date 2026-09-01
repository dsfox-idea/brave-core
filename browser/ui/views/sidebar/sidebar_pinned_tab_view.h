/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/browser/ui/views/tabs/accent_color/brave_tab_accent_types.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/models/image_model.h"

// A sidebar entry for a pinned tab. It is a sidebar button like any other,
// plus the container accent the same tab wears in the tab strip: the outline,
// and the badge with the container's icon.
//
// It computes none of that. The owner hands it what the tab strip answers for
// the mirrored tab, so the two surfaces cannot drift apart - which is what
// "the same as in the tab strip" has to mean.
class SidebarPinnedTabView : public SidebarItemView {
  METADATA_HEADER(SidebarPinnedTabView, SidebarItemView)

 public:
  explicit SidebarPinnedTabView(const std::u16string& accessible_name);
  ~SidebarPinnedTabView() override;

  SidebarPinnedTabView(const SidebarPinnedTabView&) = delete;
  SidebarPinnedTabView& operator=(const SidebarPinnedTabView&) = delete;

  // |colors| empty means the tab is in no container and the entry is drawn
  // plain. |icon| is the container icon, already in the accent colour.
  void SetContainerAccent(std::optional<TabAccentColors> colors,
                          ui::ImageModel icon);

  // Growser-150: the entry of the tab the user is looking at carries a light
  // on the sidebar's right edge. SidebarItemView::SetActiveState is not
  // virtual, so the state is recorded here as well as passed on.
  void SetActiveTab(bool active);
  bool is_active_tab() const { return is_active_tab_; }

  const std::optional<TabAccentColors>& accent_colors() const {
    return accent_colors_;
  }
  bool has_accent_icon() const { return has_accent_icon_; }

  // The accent has to be on a layer of its own to sit above the ink drop; a
  // test holds it to that, because losing the layer is invisible in the code
  // and shows up only as a washed-out entry after a hover.
  bool IsAccentOnItsOwnLayer() const;

  // The accent overlay, for tests that want to look at what it draws: it is on
  // a layer, so it is absent from any capture of this view's canvas.
  views::View* GetAccentOverlayForTesting();  // IN-TEST

  // views::View:
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void Layout(PassKey key) override;

 private:
  class AccentOverlayView;

  raw_ptr<AccentOverlayView> accent_overlay_ = nullptr;
  std::optional<TabAccentColors> accent_colors_;
  bool is_active_tab_ = false;
  bool has_accent_icon_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_
