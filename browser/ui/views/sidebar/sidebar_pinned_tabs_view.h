/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TABS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TABS_VIEW_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_drag_context.h"
#include "brave/browser/ui/views/sidebar/sidebar_pinned_tab_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/prefs/pref_member.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/context_menu_controller.h"
#include "ui/views/view.h"

class BraveBrowser;
class BraveTabStrip;
class Tab;

namespace content {
class WebContents;
}  // namespace content

// The pinned tabs the sidebar hosts, at the top of the sidebar.
//
// The block hosts as many pinned tabs as fit by height and no more: entries
// keep a fixed height and are never squeezed to let one more in. Whatever does
// not fit stays on the tab strip, and the count is republished on every layout,
// so resizing the window moves entries between the two surfaces on its own.
//
// It only ever hosts anything when the sidebar is permanently visible and the
// tab strip is horizontal - see IsHostingEnabled().
class SidebarPinnedTabsView : public views::View,
                              public TabStripModelObserver,
                              public views::ContextMenuController,
                              public SidebarPinnedTabView::DragDelegate {
  METADATA_HEADER(SidebarPinnedTabsView, views::View)

 public:
  explicit SidebarPinnedTabsView(BraveBrowser* browser);
  ~SidebarPinnedTabsView() override;

  SidebarPinnedTabsView(const SidebarPinnedTabsView&) = delete;
  SidebarPinnedTabsView& operator=(const SidebarPinnedTabsView&) = delete;

  // How many pinned tabs this view is showing right now. The tab strip skips
  // exactly this many leading pinned tabs.
  int hosted_count() const { return hosted_count_; }

  // The tab whose context menu an entry's right click opens; null when the
  // entry does not correspond to a pinned tab any more.
  Tab* GetTabForEntryForTesting(size_t entry_index);  // IN-TEST
  SidebarPinnedTabView* GetEntryForTesting(size_t entry_index);  // IN-TEST

  // Growser-165: whether the gesture in progress has become a drag. A test
  // that only looks at the model afterwards cannot say whether a drag was
  // never recognised or was recognised and dropped nowhere.
  bool IsDraggingForTesting() const { return dragging_; }  // IN-TEST
  bool IsHandedOffForTesting() const { return handed_off_; }  // IN-TEST

  // views::View:
  void Layout(PassKey) override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
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

  // views::ContextMenuController:
  void ShowContextMenuForViewImpl(
      views::View* source,
      const gfx::Point& point,
      ui::mojom::MenuSourceType source_type) override;

  // SidebarPinnedTabView::DragDelegate:
  void OnEntryMousePressed(SidebarPinnedTabView* entry,
                           const ui::MouseEvent& event) override;
  bool OnEntryMouseDragged(SidebarPinnedTabView* entry,
                           const ui::MouseEvent& event) override;
  void OnEntryMouseReleased(SidebarPinnedTabView* entry,
                            const ui::MouseEvent& event) override;
  void OnEntryDragCancelled(SidebarPinnedTabView* entry) override;

 private:
  // The Tab the entry mirrors, looked up in the tab strip.
  Tab* GetTabForEntry(size_t entry_index) const;
  BraveTabStrip* GetBraveTabStrip() const;

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

  // Growser-165: the drag, while the pointer is still in the sidebar.
  // The gap the entry would drop into, counted in visible entries, so 0 is
  // above the first and `hosted_count_` is below the last.
  std::optional<size_t> CalculateDragIndicatorIndex(const gfx::Point& p) const;
  void DrawDragIndicator(std::optional<size_t> index);
  void ClearDragIndicator();
  void ResetDrag();

  // Growser-165: the pointer has left the sidebar sideways, so the gesture
  // stops being ours. Runs from a posted task, never inline: it stops the
  // block hosting anything, which destroys the very entry whose event is on
  // the stack.
  void HandOffToTabStrip(size_t entry_index, gfx::Point point_in_screen);

  // The handed-over drag is over. Hosting resumes either way; `unpin` says the
  // tab was dropped on a tab strip and is an ordinary tab now.
  void OnHandedOffDragEnded(base::WeakPtr<content::WebContents> dragged,
                            bool unpin);

  raw_ptr<BraveBrowser> browser_ = nullptr;
  std::vector<raw_ptr<SidebarPinnedTabView>> entries_;
  int hosted_count_ = 0;

  SidebarItemDragContext drag_context_;
  // Where the button was pressed, in this view's coordinates.
  gfx::Point press_point_;
  bool dragging_ = false;
  // True from the moment the gesture is handed to the tab strip. The block
  // hosts nothing while it is set, so the dragged tab has real bounds in the
  // strip for TabDragController to work with.
  bool handed_off_ = false;

  BooleanPrefMember show_pinned_tabs_;
  BooleanPrefMember vertical_tabs_enabled_;
  IntegerPrefMember sidebar_show_option_;

  base::WeakPtrFactory<SidebarPinnedTabsView> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDEBAR_SIDEBAR_PINNED_TABS_VIEW_H_
