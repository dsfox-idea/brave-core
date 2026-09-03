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
  // Growser-165: the block owns the gesture, not the entry. An entry cannot
  // tell a click from the start of a drag by itself - that answer is about
  // where the pointer went relative to the other entries - so it forwards its
  // mouse events and lets the block decide.
  class DragDelegate {
   public:
    virtual void OnEntryMousePressed(SidebarPinnedTabView* entry,
                                     const ui::MouseEvent& event) = 0;
    // True once the gesture has become a drag.
    virtual bool OnEntryMouseDragged(SidebarPinnedTabView* entry,
                                     const ui::MouseEvent& event) = 0;
    virtual void OnEntryMouseReleased(SidebarPinnedTabView* entry,
                                      const ui::MouseEvent& event) = 0;
    virtual void OnEntryDragCancelled(SidebarPinnedTabView* entry) = 0;

   protected:
    virtual ~DragDelegate() = default;
  };

  explicit SidebarPinnedTabView(const std::u16string& accessible_name);
  ~SidebarPinnedTabView() override;

  void set_drag_delegate(DragDelegate* delegate) { drag_delegate_ = delegate; }

  SidebarPinnedTabView(const SidebarPinnedTabView&) = delete;
  SidebarPinnedTabView& operator=(const SidebarPinnedTabView&) = delete;

  // |colors| empty means the tab is in no container and the entry is drawn
  // plain. |icon| is the container icon, already in the accent colour.
  void SetContainerAccent(std::optional<TabAccentColors> colors,
                          ui::ImageModel icon);

  // Growser-150: the block paints a light on the sidebar's right edge for the
  // active entry, and needs to know which one that is.
  // SidebarItemView::SetActiveState is not virtual, so the state is recorded
  // here as well as passed on.
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
  void Layout(PassKey key) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool OnMouseDragged(const ui::MouseEvent& event) override;
  void OnMouseReleased(const ui::MouseEvent& event) override;
  void OnMouseCaptureLost() override;

  // views::Button:
  // Growser-165: a gesture that turned into a drag is not a click, so
  // releasing over the entry must not also activate its tab.
  bool IsTriggerableEvent(const ui::Event& e) override;

 private:
  class AccentOverlayView;

  raw_ptr<AccentOverlayView> accent_overlay_ = nullptr;
  std::optional<TabAccentColors> accent_colors_;
  bool is_active_tab_ = false;
  bool has_accent_icon_ = false;

  raw_ptr<DragDelegate> drag_delegate_ = nullptr;
  bool dragging_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TAB_VIEW_H_
