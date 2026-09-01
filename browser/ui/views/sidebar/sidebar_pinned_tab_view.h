/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_

#include <optional>
#include <string>

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

  const std::optional<TabAccentColors>& accent_colors() const {
    return accent_colors_;
  }
  bool has_accent_icon() const { return !accent_icon_.IsEmpty(); }

  // views::View:
  void OnPaintBorder(gfx::Canvas* canvas) override;
  void PaintChildren(const views::PaintInfo& paint_info) override;

 private:
  std::optional<TabAccentColors> accent_colors_;
  ui::ImageModel accent_icon_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_
