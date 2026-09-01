/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_pinned_tabs_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_pinned_tabs_layout.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
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
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/controls/separator.h"
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
// What the block spends above its first entry: the separator and its own top
// margin. The margin below the separator collapses into the entry's.
constexpr int kLeadingHeight =
    views::Separator::kThickness + SidebarButtonView::kMargin;

}  // namespace

SidebarPinnedTabsView::SidebarPinnedTabsView(BraveBrowser* browser)
    : browser_(browser) {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
                       views::BoxLayout::Orientation::kVertical))
      ->SetCollapseMarginsSpacing(true);

  separator_ = AddChildView(std::make_unique<views::Separator>());
  separator_->SetColorId(kColorBraveVerticalTabSeparator);
  separator_->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(SidebarButtonView::kMargin, SidebarButtonView::kMargin));

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

  // The separator only divides two non-empty sides, exactly like the one the
  // tab strip draws between pinned and unpinned tabs.
  separator_->SetVisible(hosted > 0);
  for (size_t i = 0; i < entries_.size(); ++i) {
    entries_[i]->SetVisible(static_cast<int>(i) < hosted);
  }

  LayoutSuperclass<views::View>(this);

  PublishHostedCount(hosted);
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
  for (SidebarItemView* entry : entries_) {
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
    auto* entry = AddChildView(std::make_unique<SidebarItemView>(u""));
    entry->set_context_menu_controller(this);
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

  SidebarItemView* entry = entries_[entry_index];

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

  entry->SetActiveState(model->active_index() == index);
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

Tab* SidebarPinnedTabsView::GetTabForEntryForTesting(size_t entry_index) {
  return GetTabForEntry(entry_index);
}

Tab* SidebarPinnedTabsView::GetTabForEntry(size_t entry_index) const {
  if (entry_index >= entries_.size() ||
      static_cast<int>(entry_index) >=
          browser_->tab_strip_model()->IndexOfFirstNonPinnedTab()) {
    return nullptr;
  }

  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  if (!browser_view) {
    return nullptr;
  }

  auto* tab_strip = views::AsViewClass<TabStrip>(
      browser_view->GetViewByID(VIEW_ID_TAB_STRIP));
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
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser_);
  auto* tab_strip = views::AsViewClass<TabStrip>(
      browser_view->GetViewByID(VIEW_ID_TAB_STRIP));
  tab_strip->ShowContextMenuForTab(tab, point, source_type);
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
