/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_pinned_tabs_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_pinned_tabs_layout.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_pinned_tab_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_user_gesture_details.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/image_operations.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"

namespace {

constexpr gfx::Size kIconSize(SidebarButtonView::kExternalIconSize,
                              SidebarButtonView::kExternalIconSize);

// Entries are sidebar buttons, so they carry the sidebar button's height and
// margins. Margin collapsing makes the gap between two of them one margin.
constexpr int kEntryHeight = SidebarButtonView::kSidebarButtonSize;
constexpr int kSpacing = SidebarButtonView::kMargin;
// Growser-149: the block sits at the very top of the sidebar now, so it has
// nothing above its first entry - the divider moved down to the block that
// carries bookmarks, reading list and the gear.
constexpr int kLeadingHeight = 0;

// Growser-150: the active tab's light. A thin bar, and a glow that is a real
// blur rather than a gradient rectangle - a rectangle has hard top and bottom
// edges and reads as a block, which is what the first attempt looked like.
//
// The glow is blurred from a shape wider than the bar: blurring the 2px bar
// alone spreads far too little mass to see (measured: it peaked at 18% alpha
// and died within 10px, well before reaching the icon). It is not offset
// either - pushing the blur left moves its bright core off the bar, which
// reads as a separate smudge rather than as light coming from the edge.
constexpr int kActiveBarWidth = 2;
constexpr int kActiveGlowSourceWidth = 10;
constexpr int kActiveGlowBlur = 22;
constexpr int kActiveGlowOffset = 0;
constexpr SkAlpha kActiveGlowAlpha = 0xB3;

}  // namespace

SidebarPinnedTabsView::SidebarPinnedTabsView(BraveBrowser* browser)
    : browser_(browser) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical))
      ->SetCollapseMarginsSpacing(true);

  auto* prefs = browser_->GetProfile()->GetPrefs();
  auto on_setting_changed = base::BindRepeating(
      &SidebarPinnedTabsView::OnSettingChanged, base::Unretained(this));
  show_pinned_tabs_.Init(sidebar::kSidebarShowPinnedTabs, prefs,
                         on_setting_changed);
  vertical_tabs_enabled_.Init(brave_tabs::kVerticalTabsEnabled, prefs,
                              on_setting_changed);
  sidebar_show_option_.Init(sidebar::kSidebarShowOption, prefs,
                            on_setting_changed);

  browser_->tab_strip_model()->AddObserver(this);
  RebuildEntries();
}

// TabStripModelObserver detaches itself, and by this point the browser may
// already be on its way out - hence nothing here.
SidebarPinnedTabsView::~SidebarPinnedTabsView() = default;

void SidebarPinnedTabsView::Layout(PassKey) {
  const int capacity =
      IsHostingEnabled() ? sidebar::CalculatePinnedTabsCapacity(
                               height(), kEntryHeight, kSpacing, kLeadingHeight)
                         : 0;
  const int hosted = std::min(capacity, static_cast<int>(entries_.size()));

  for (size_t i = 0; i < entries_.size(); ++i) {
    entries_[i]->SetVisible(static_cast<int>(i) < hosted);
  }

  LayoutSuperclass<views::View>(this);

  PublishHostedCount(hosted);
}

// Growser-150: the light for the tab the user is looking at - a 2px bar at the
// sidebar's right edge, as tall as the entry's icon square, with a real glow
// around it (a Skia shadow, offset to the left so it reaches under the icon).
//
// Painted by the block rather than by the entry, for two reasons: the block
// paints before its children, so the glow lands under the favicons, and it is
// tall enough that the glow fades out on its own instead of being cut off at
// an entry's 32px edge.
void SidebarPinnedTabsView::OnPaintBackground(gfx::Canvas* canvas) {
  views::View::OnPaintBackground(canvas);

  const ui::ColorProvider* color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  for (SidebarPinnedTabView* entry : entries_) {
    if (!entry->GetVisible() || !entry->is_active_tab()) {
      continue;
    }

    // The container's colour when there is one, so the light and the container
    // accent read as one thing rather than two decorations.
    const SkColor color =
        entry->accent_colors().has_value()
            ? entry->accent_colors()->border_color
            : color_provider->GetColor(kColorSidebarButtonPressed);

    // As tall as the icon square the entry draws, not as the favicon inside it.
    gfx::Rect icon = entry->GetContentsBounds();
    icon.Offset(entry->bounds().OffsetFromOrigin());

    // The glow first, blurred out of a wider shape and pushed left so it
    // reaches under the icon. The source shape itself is not painted - only
    // its shadow is, which is what a glow is.
    cc::PaintFlags glow_flags;
    glow_flags.setAntiAlias(true);
    glow_flags.setStyle(cc::PaintFlags::kFill_Style);
    glow_flags.setColor(SK_ColorTRANSPARENT);
    glow_flags.setLooper(gfx::CreateShadowDrawLooper(
        {gfx::ShadowValue(gfx::Vector2d(-kActiveGlowOffset, 0), kActiveGlowBlur,
                          SkColorSetA(color, kActiveGlowAlpha))}));
    canvas->DrawRoundRect(
        gfx::RectF(width() - kActiveGlowSourceWidth, icon.y(),
                   kActiveGlowSourceWidth, icon.height()),
        kActiveGlowSourceWidth / 2.0f, glow_flags);

    cc::PaintFlags bar_flags;
    bar_flags.setAntiAlias(true);
    bar_flags.setStyle(cc::PaintFlags::kFill_Style);
    bar_flags.setColor(color);
    canvas->DrawRoundRect(
        gfx::RectF(width() - kActiveBarWidth, icon.y(), kActiveBarWidth,
                   icon.height()),
        kActiveBarWidth / 2.0f, bar_flags);
    return;
  }
}

gfx::Size SidebarPinnedTabsView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  if (!IsHostingEnabled() || entries_.empty()) {
    return {0, 0};
  }

  // Ask for everything: the layout hands over what it can spare, and Layout()
  // turns that height into the number of entries that fit.
  const int width = kEntryHeight + kSpacing * 2;
  const int height =
      kLeadingHeight + kSpacing +
      static_cast<int>(entries_.size()) * (kEntryHeight + kSpacing);
  return {width, height};
}

void SidebarPinnedTabsView::OnThemeChanged() {
  views::View::OnThemeChanged();

  for (size_t i = 0; i < entries_.size(); ++i) {
    UpdateEntry(i);
  }
}

void SidebarPinnedTabsView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (IsWindowClosing()) {
    return;
  }

  const size_t pinned_count = static_cast<size_t>(
      browser_->tab_strip_model()->IndexOfFirstNonPinnedTab());
  if (pinned_count != entries_.size()) {
    RebuildEntries();
    return;
  }

  // Same pinned tabs, but one of them may have been replaced, moved or
  // activated: entry i always mirrors model index i.
  for (size_t i = 0; i < entries_.size(); ++i) {
    UpdateEntry(i);
  }
}

void SidebarPinnedTabsView::OnTabPinnedStateChanged(tabs::TabInterface* tab,
                                                    int index) {
  if (IsWindowClosing()) {
    return;
  }

  RebuildEntries();
}

void SidebarPinnedTabsView::OnTabChangedAt(tabs::TabInterface* tab,
                                           int index,
                                           TabChangeType change_type) {
  if (index >= 0 && static_cast<size_t>(index) < entries_.size()) {
    UpdateEntry(static_cast<size_t>(index));
  }
}

bool SidebarPinnedTabsView::IsHostingEnabled() const {
  // Growser-165: the gesture belongs to the tab strip now, and the tab it is
  // carrying has to be laid out there - a tab the sidebar hosts is drawn
  // closed in the strip, and a tab with no bounds is not something a drag can
  // pick up.
  if (handed_off_) {
    return false;
  }

  if (!show_pinned_tabs_.GetValue()) {
    return false;
  }

  // Vertical tabs already show pinned tabs in a column of their own.
  auto* vertical_tab_controller = VerticalTabController::FromBrowser(browser_);
  if (vertical_tab_controller &&
      vertical_tab_controller->ShouldShowBraveVerticalTabs()) {
    return false;
  }

  // Anything but "always" means the sidebar comes and goes. Hosting a pinned
  // tab there would either hide it with the sidebar or make it jump between
  // the two surfaces as the sidebar slides in and out.
  return sidebar_show_option_.GetValue() ==
         static_cast<int>(
             sidebar::SidebarService::ShowSidebarOption::kShowAlways);
}

// Every tab is on its way out, so there is nothing to mirror. Rebuilding here
// would tear down entry views in the middle of the window's own teardown, and
// a view destroyed while the accessibility layer still holds a reference to it
// leaves a ghost platform node behind.
bool SidebarPinnedTabsView::IsWindowClosing() const {
  return browser_->tab_strip_model()->closing_all();
}

void SidebarPinnedTabsView::RebuildEntries() {
  // Growser-165: the drag holds a pointer to one of these. A pinned tab
  // appearing or leaving while the pointer is down is rare and not worth
  // following - dropping the gesture is.
  ResetDrag();

  for (SidebarPinnedTabView* entry : entries_) {
    RemoveChildViewT(entry);
  }
  entries_.clear();

  // A block that is not hosting keeps no entries at all: fewer views to build,
  // and nothing to destroy when the sidebar or the window goes away.
  const int pinned_count =
      IsHostingEnabled()
          ? browser_->tab_strip_model()->IndexOfFirstNonPinnedTab()
          : 0;
  for (int i = 0; i < pinned_count; ++i) {
    auto* entry = AddChildView(std::make_unique<SidebarPinnedTabView>(u""));
    entry->set_context_menu_controller(this);
    entry->set_drag_delegate(this);  // Growser-165
    entry->SetCallback(
        base::BindRepeating(&SidebarPinnedTabsView::OnEntryPressed,
                            base::Unretained(this), static_cast<size_t>(i)));
    entries_.push_back(entry);
    UpdateEntry(static_cast<size_t>(i));
  }

  PreferredSizeChanged();
}

void SidebarPinnedTabsView::UpdateEntry(size_t entry_index) {
  CHECK_LT(entry_index, entries_.size());

  auto* model = browser_->tab_strip_model();
  const int index = static_cast<int>(entry_index);
  content::WebContents* contents = model->GetWebContentsAt(index);
  if (!contents) {
    return;
  }

  SidebarPinnedTabView* entry = entries_[entry_index];

  std::u16string title = contents->GetTitle();
  if (title.empty()) {
    title = base::UTF8ToUTF16(contents->GetVisibleURL().spec());
  }
  entry->SetAccessibleName(title);
  entry->SetTooltipText(title);

  auto* favicon_driver =
      favicon::ContentFaviconDriver::FromWebContents(contents);
  if (favicon_driver) {
    const gfx::Image favicon = favicon_driver->GetFavicon();
    if (!favicon.IsEmpty()) {
      entry->SetImageModel(
          views::Button::STATE_NORMAL,
          ui::ImageModel::FromImageSkia(
              gfx::ImageSkiaOperations::CreateResizedImage(
                  favicon.AsImageSkia(), skia::ImageOperations::RESIZE_BEST,
                  kIconSize)));
    }
  }

  entry->SetActiveTab(model->active_index() == index);

  // Whatever the tab strip would draw for this tab, drawn here too. Asked of
  // the strip rather than worked out again, so the two cannot disagree.
  auto* tab_strip = GetBraveTabStrip();
  Tab* tab = GetTabForEntry(entry_index);
  if (tab_strip && tab && tab_strip->ShouldPaintTabAccent(tab)) {
    entry->SetContainerAccent(tab_strip->GetTabAccentColors(tab),
                              tab_strip->GetTabAccentIcon(tab));
  } else {
    entry->SetContainerAccent(std::nullopt, ui::ImageModel());
  }
}

void SidebarPinnedTabsView::OnEntryPressed(size_t entry_index) {
  auto* model = browser_->tab_strip_model();
  const int index = static_cast<int>(entry_index);
  if (index >= model->IndexOfFirstNonPinnedTab()) {
    return;
  }

  model->ActivateTabAt(index,
                       TabStripUserGestureDetails(
                           TabStripUserGestureDetails::GestureType::kOther));
}

SidebarPinnedTabView* SidebarPinnedTabsView::GetEntryForTesting(
    size_t entry_index) {
  return entry_index < entries_.size() ? entries_[entry_index].get() : nullptr;
}

Tab* SidebarPinnedTabsView::GetTabForEntryForTesting(size_t entry_index) {
  return GetTabForEntry(entry_index);
}

BraveTabStrip* SidebarPinnedTabsView::GetBraveTabStrip() const {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  if (!browser_view) {
    return nullptr;
  }

  return views::AsViewClass<BraveTabStrip>(
      browser_view->GetViewByID(VIEW_ID_TAB_STRIP));
}

Tab* SidebarPinnedTabsView::GetTabForEntry(size_t entry_index) const {
  if (entry_index >= entries_.size() ||
      static_cast<int>(entry_index) >=
          browser_->tab_strip_model()->IndexOfFirstNonPinnedTab()) {
    return nullptr;
  }

  auto* tab_strip = GetBraveTabStrip();
  if (!tab_strip) {
    return nullptr;
  }

  // Pinned tabs lead the model, so the entry index is the tab index. The tab is
  // laid out with no width and is not painted while the sidebar hosts it, which
  // the menu does not care about: it works off the model index.
  return tab_strip->tab_at(static_cast<int>(entry_index));
}

void SidebarPinnedTabsView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::mojom::MenuSourceType source_type) {
  const auto entry = std::ranges::find(entries_, source);
  if (entry == entries_.end()) {
    return;
  }

  Tab* tab = GetTabForEntry(
      static_cast<size_t>(std::distance(entries_.begin(), entry)));
  if (!tab) {
    return;
  }

  // Hand it to the tab strip rather than building a menu here: "the same menu
  // the tab has in the strip" is the requirement, and this is that menu - the
  // model, the Brave items and the delegate that knows what Brave's own
  // commands mean (BraveBrowserTabStripController) all come with it.
  GetBraveTabStrip()->ShowContextMenuForTab(tab, point, source_type);
}

void SidebarPinnedTabsView::OnSettingChanged() {
  // The entries themselves follow the setting now, so this both creates and
  // drops them.
  RebuildEntries();

  // Hosting can only be turned off from here, and an invisible or zero-height
  // block is never laid out again - so hand the tabs back right away rather
  // than waiting for a layout that may not come.
  if (!IsHostingEnabled()) {
    PublishHostedCount(0);
  }

  InvalidateLayout();
}

void SidebarPinnedTabsView::VisibilityChanged(views::View* starting_from,
                                              bool is_visible) {
  // The sidebar hides in fullscreen, and a hidden sidebar hosts nothing: every
  // pinned tab belongs to the tab strip again.
  if (!is_visible) {
    PublishHostedCount(0);
  }
}

// Growser-165: the drag inside the sidebar.
//
// Entry i mirrors model index i, so a reorder is nothing but a move in the tab
// strip model - the entries never change places, they redraw with the tab that
// is now at their index. Both indices are inside the pinned range, so the
// model's own ConstrainMoveIndex() has nothing to clamp.
void SidebarPinnedTabsView::OnEntryMousePressed(SidebarPinnedTabView* entry,
                                                const ui::MouseEvent& event) {
  const auto found = std::ranges::find(entries_, entry);
  if (found == entries_.end()) {
    return;
  }

  press_point_ = event.location();
  views::View::ConvertPointToTarget(entry, this, &press_point_);
  drag_context_.set_source(entry);
  drag_context_.set_source_index(
      static_cast<size_t>(std::distance(entries_.begin(), found)));
}

bool SidebarPinnedTabsView::OnEntryMouseDragged(SidebarPinnedTabView* entry,
                                                const ui::MouseEvent& event) {
  if (!drag_context_.source_index() || drag_context_.source() != entry) {
    return false;
  }

  gfx::Point p = event.location();
  views::View::ConvertPointToTarget(entry, this, &p);

  if (!dragging_ && !SidebarItemDragContext::CanStartDrag(press_point_, p)) {
    return false;
  }
  dragging_ = true;

  // Sideways out of the sidebar and the gesture stops being ours: from here it
  // is a tab drag like any other, with the tear-off window and the other
  // windows' strips that come with it. Only a horizontal exit counts - below
  // the block are Brave's own sidebar buttons, which take no part in this.
  //
  // Posted rather than done here: handing over stops the block hosting
  // anything, which destroys the entry whose event is on this stack.
  if (!handed_off_ &&
      (p.x() < -SidebarButtonView::kMargin ||
       p.x() > width() + SidebarButtonView::kMargin)) {
    // No gap to fall into any more. The button can be released before the
    // posted task runs, and without this the release would reorder to
    // whichever gap the pointer was last over inside the block.
    DrawDragIndicator(std::nullopt);

    gfx::Point in_screen = event.location();
    views::View::ConvertPointToScreen(entry, &in_screen);
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&SidebarPinnedTabsView::HandOffToTabStrip,
                                  weak_factory_.GetWeakPtr(),
                                  *drag_context_.source_index(), in_screen));
    return true;
  }

  DrawDragIndicator(CalculateDragIndicatorIndex(p));
  return true;
}

void SidebarPinnedTabsView::HandOffToTabStrip(size_t entry_index,
                                              gfx::Point point_in_screen) {
  // The button can be let go before this task runs, and a drag session with
  // no button held down is worse than no drag at all: nothing would ever end
  // it.
  if (!dragging_) {
    return;
  }

  auto* tab_strip = GetBraveTabStrip();
  auto* model = browser_->tab_strip_model();
  const int index = static_cast<int>(entry_index);
  if (!tab_strip || index >= model->IndexOfFirstNonPinnedTab()) {
    return;
  }

  // Stop hosting before anything else, so the strip lays the tab out for real.
  handed_off_ = true;
  ResetDrag();
  RebuildEntries();
  PublishHostedCount(0);

  // ...and make the strip catch up now: MaybeStartDrag refuses outright while
  // the strip is animating, and the tab has no bounds until it has laid out.
  tab_strip->StopAnimating();

  // The drag set is built from the selection model, and
  // TabDragContextImpl::MaybeStartDrag CHECKs that the source is in it.
  // Pressing a tab in the strip selects it, so this is only what the same
  // gesture would have done over there.
  model->ActivateTabAt(index,
                       TabStripUserGestureDetails(
                           TabStripUserGestureDetails::GestureType::kOther));

  // TabDragController takes capture in Init(), and it cannot while our own
  // widget still holds it.
  if (GetWidget()) {
    GetWidget()->ReleaseCapture();
  }

  content::WebContents* contents = model->GetWebContentsAt(index);
  if (!tab_strip->StartDragFromSidebar(
          tab_strip->tab_at(index), point_in_screen,
          base::BindOnce(&SidebarPinnedTabsView::OnHandedOffDragEnded,
                         weak_factory_.GetWeakPtr(),
                         contents ? contents->GetWeakPtr()
                                  : base::WeakPtr<content::WebContents>()))) {
    // Nobody took the gesture, so nobody will hand the tabs back either.
    handed_off_ = false;
    RebuildEntries();
    InvalidateLayout();
  }
}

void SidebarPinnedTabsView::OnHandedOffDragEnded(
    base::WeakPtr<content::WebContents> dragged,
    bool unpin) {
  handed_off_ = false;

  // Dropped on a tab strip, so it is an ordinary tab now - any strip,
  // including this window's own. That case is the reason the rule exists: this
  // window hosts pinned tabs in its sidebar, so a tab dropped on its own strip
  // while still pinned would be pulled straight back into the sidebar the user
  // had just dragged it out of.
  if (unpin && dragged) {
    auto* browser =
        GlobalBrowserCollection::GetInstance()->FindBrowserWithTab(
            dragged.get());
    if (browser) {
      TabStripModel* model = browser->GetTabStripModel();
      const int index = model->GetIndexOfWebContents(dragged.get());
      if (index != TabStripModel::kNoTab && model->IsTabPinned(index)) {
        model->SetTabPinned(index, false);
      }
    }
  }

  RebuildEntries();
  InvalidateLayout();
}

void SidebarPinnedTabsView::OnEntryMouseReleased(SidebarPinnedTabView* entry,
                                                 const ui::MouseEvent& event) {
  if (!dragging_) {
    ResetDrag();
    return;
  }

  if (drag_context_.ShouldMoveItem()) {
    const int from = static_cast<int>(*drag_context_.source_index());
    const int to = static_cast<int>(drag_context_.GetTargetIndex());
    browser_->tab_strip_model()->MoveWebContentsAt(from, to,
                                                  /*select_after_move=*/false);
  }

  ResetDrag();
}

void SidebarPinnedTabsView::OnEntryDragCancelled(SidebarPinnedTabView* entry) {
  ResetDrag();
}

std::optional<size_t> SidebarPinnedTabsView::CalculateDragIndicatorIndex(
    const gfx::Point& p) const {
  if (hosted_count_ <= 0) {
    return std::nullopt;
  }

  // The block reaches exactly as far as the entries it is showing, and no
  // further: the rest of its height is room it asked for, and below it are
  // Brave's own sidebar buttons, which take no part in this. A pointer outside
  // that has no gap to fall into, so nothing is drawn and nothing moves.
  gfx::Rect region = entries_.front()->bounds();
  region.Union(entries_[static_cast<size_t>(hosted_count_) - 1]->bounds());
  region.Outset(SidebarButtonView::kMargin);
  if (!region.Contains(p)) {
    return std::nullopt;
  }

  for (int i = 0; i < hosted_count_; ++i) {
    const gfx::Rect bounds = entries_[static_cast<size_t>(i)]->bounds();
    if (p.y() < bounds.CenterPoint().y()) {
      return static_cast<size_t>(i);
    }
  }
  return static_cast<size_t>(hosted_count_);
}

void SidebarPinnedTabsView::DrawDragIndicator(std::optional<size_t> index) {
  ClearDragIndicator();
  drag_context_.set_drag_indicator_index(index);

  // Right before or right after the entry being dragged is where it already
  // is, so there is nothing to show.
  const auto source = drag_context_.source_index();
  if (!index || !source || hosted_count_ <= 0 || *source == *index ||
      *source + 1 == *index) {
    return;
  }

  // The gap is drawn as the top border of the entry below it, except past the
  // last entry, where it is that entry's bottom border.
  const bool top = *index != static_cast<size_t>(hosted_count_);
  entries_[top ? *index : *index - 1]->DrawHorizontalBorder(top);
}

void SidebarPinnedTabsView::ClearDragIndicator() {
  for (SidebarPinnedTabView* entry : entries_) {
    entry->ClearHorizontalBorder();
  }
}

void SidebarPinnedTabsView::ResetDrag() {
  ClearDragIndicator();
  drag_context_.Reset();
  dragging_ = false;
}

void SidebarPinnedTabsView::PublishHostedCount(int count) {
  if (hosted_count_ == count) {
    return;
  }
  hosted_count_ = count;

  if (auto* controller = browser_->GetFeatures().sidebar_controller()) {
    controller->SetPinnedTabCountHostedBySidebar(count);
  }
}

BEGIN_METADATA(SidebarPinnedTabsView)
END_METADATA
