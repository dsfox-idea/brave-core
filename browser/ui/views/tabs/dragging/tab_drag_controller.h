/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_

#include <optional>
#include <vector>

#include "base/functional/callback.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"

class BraveTabDragController : public TabDragController {
 public:
  // Growser-165: a drag can begin on a pinned tab's entry in the sidebar
  // rather than on a tab. The sidebar hands the gesture over and then needs to
  // hear how it ended: it resumes hosting either way, and the tab loses its pin
  // unless the drag tore off a window of its own.
  //
  // `unpin` is the whole answer; what to do about it is the sidebar's business,
  // not this class's.
  using DragEndedCallback = base::OnceCallback<void(bool unpin)>;

  // Set immediately before TabStrip::MaybeStartDrag() and consumed by the next
  // instance's constructor. Static because that instance is created inside
  // Chromium's own TabDragContextImpl, which exposes no way to reach it - and
  // there is only ever one drag session, one synchronous step after the mark.
  // Pass an empty callback to take the mark back if no drag started.
  static void MarkNextDragAsStartedFromSidebar(DragEndedCallback on_ended);

  BraveTabDragController();
  ~BraveTabDragController() override;

  // TabDragController:
  void EndDrag(EndDragReason reason) override;
  [[nodiscard]] Liveness Init(TabDragContext* source_context,
                              TabSlotView* source_view,
                              const std::vector<TabSlotView*>& dragging_views,
                              const gfx::Point& offset_from_source_view,
                              ui::ListSelectionModel initial_selection_model,
                              ui::mojom::DragEventSource event_source) override;
  void StartDraggingTabsSession(bool initial_move,
                                gfx::Point start_point_in_screen) override;
  void DetachAndAttachToNewContext(ReleaseCapture release_capture,
                                   TabDragContext* target_context) override;

 private:
  gfx::Point offset_from_first_dragged_view_;
  bool is_showing_vertical_tabs_ = false;
  DragEndedCallback sidebar_drag_ended_;  // Growser-165

  BraveVerticalTabStripRegionView::ScopedStateResetter
      vertical_tab_state_resetter_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_
