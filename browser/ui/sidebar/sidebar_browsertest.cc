/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <optional>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/i18n/rtl.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/files/file_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_features.h"
#include "brave/browser/ui/sidebar/sidebar_browsertest_base.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/sidebar/sidebar_web_panel_controller.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/frame/split_view/brave_contents_container_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view.h"
#include "brave/browser/ui/views/frame/split_view/brave_multi_contents_view_mini_toolbar.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_header.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_resize_area.h"
#include "brave/browser/ui/views/side_panel/side_panel_utils.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_pinned_tabs_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_pinned_tab_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "brave/browser/ui/views/toolbar/side_panel_button.h"
#include "brave/common/pref_names.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/constants.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/sidebar/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tabs/public/tab_interface.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/multi_contents_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/ui_base_features.h"
#include "ui/compositor/layer.h"
#include "ui/display/screen.h"
#include "ui/display/test/test_screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/animation/animation.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/view_utils.h"
#include "ui/views/controls/menu/menu_controller.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/views/paint_info.h"
#include "ui/compositor/canvas_painter.h"
#include "ui/views/widget/widget_utils.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/common/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/common/features.h"
#endif

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Optional;

namespace sidebar {

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, BasicTest) {
  EXPECT_TRUE(!!GetSidePanelToolbarButton()->context_menu_controller());

  // Initially, active index is not set.
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  // Check sidebar UI is initalized properly.
  EXPECT_TRUE(!!controller()->sidebar());

  // `active_index()` is updated synchronously when the panel is shown/closed,
  // but `IsSidePanelShowing()` only flips once the entry finishes loading
  // asynchronously. Toggling again before the panel is actually showing makes
  // SidePanelCoordinator::Toggle() re-show instead of close, so wait on both.
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  chrome::BrowserCommandController::From(browser())->ExecuteCommand(
      IDC_TOGGLE_SIDEBAR);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return !!model()->active_index() && panel_ui->IsSidePanelShowing();
  }));
  // Check active index is non-null.
  EXPECT_THAT(model()->active_index(), Ne(std::nullopt));

  chrome::BrowserCommandController::From(browser())->ExecuteCommand(
      IDC_TOGGLE_SIDEBAR);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return !model()->active_index() && !panel_ui->IsSidePanelShowing();
  }));
  // Check active index is null.
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  auto expected_count = GetDefaultItemCount();
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());

  // Activate item that opens in panel.
  const size_t first_panel_item_index = GetFirstPanelItemIndex();
  const auto& first_panel_item =
      controller()->model()->GetAllSidebarItems()[first_panel_item_index];

  controller()->ActivateItemAt(
      model()->GetIndexOf(first_panel_item.built_in_item_type));
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Optional(first_panel_item_index));
  EXPECT_TRUE(controller()->IsActiveIndex(first_panel_item_index));

  // Get first index of item that opens in a new tab (not panel).
  // Note: Web-type items (e.g., kBraveTalk) may not exist if their
  // respective features are disabled.
  const size_t first_web_item_index = GetFirstWebItemIndex();
  int active_item_index = first_panel_item_index;
  if (first_web_item_index < model()->GetAllSidebarItems().size()) {
    const auto item = model()->GetAllSidebarItems()[first_web_item_index];
    EXPECT_FALSE(item.open_in_panel);
    controller()->ActivateItemAt(first_web_item_index);
  }
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  controller()->DeactivateCurrentPanel();
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Eq(std::nullopt));

  controller()->ActivatePanelItem(first_panel_item.built_in_item_type);
  WaitUntil(
      base::BindLambdaForTesting([&]() { return !!model()->active_index(); }));
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->GetProfile());

  // Move active item to the next index to make sure it's not the first item.
  sidebar_service->MoveItem(first_panel_item_index, first_panel_item_index + 1);
  active_item_index++;
  EXPECT_THAT(model()->active_index(), Eq(active_item_index));

  // Remove Item at index 0 to change active index.
  sidebar_service->RemoveItemAt(0);
  active_item_index--;
  --expected_count;

  // Wait for sidebar item removal to complete before navigation.
  WaitUntil(base::BindLambdaForTesting([&]() {
    return model()->GetAllSidebarItems().size() == expected_count;
  }));
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());
  EXPECT_THAT(model()->active_index(), Optional(active_item_index));

  // Navigate to a non-NTP page first to ensure we can add it to sidebar
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings/")));

  // If current active tab is not NTP, we can add current url to sidebar.
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));

  // If current active tab is NTP, we can't add current url to sidebar.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), GURL("chrome://newtab/")));
  EXPECT_FALSE(CanAddCurrentActiveTabToSidebar(browser()));

  // Check |BrowserView::find_bar_host_view_| is the last child view.
  // If not, findbar dialog is not positioned properly.
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto find_bar_host_view_index =
      browser_view->GetIndexOf(browser_view->find_bar_host_view());
  EXPECT_THAT(find_bar_host_view_index,
              Optional(browser_view->children().size() - 1));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, WebTypePanelTest) {
  auto expected_count = GetDefaultItemCount();
  EXPECT_EQ(expected_count, model()->GetAllSidebarItems().size());

  // Add an item
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("chrome://settings/")));
  int current_tab_index = tab_model()->active_index();
  EXPECT_EQ(0, current_tab_index);
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  // Verify new size
  EXPECT_EQ(++expected_count, model()->GetAllSidebarItems().size());

  // Load NTP in a new tab and activate it. (tab index 1)
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  current_tab_index = tab_model()->active_index();
  EXPECT_EQ(1, current_tab_index);

  // Activate sidebar item(chrome://settings) and check existing first tab is
  // activated.
  auto items = model()->GetAllSidebarItems();
  auto iter =
      std::ranges::find(items, GURL("chrome://settings/"), &SidebarItem::url);
  EXPECT_NE(items.end(), iter);
  controller()->ActivateItemAt(std::distance(items.begin(), iter));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), iter->url);

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  // Activate second sidebar item(wallet) and check it's loaded at current tab.
  iter = std::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                           &SidebarItem::built_in_item_type);
  EXPECT_NE(items.end(), iter);
  controller()->ActivateItemAt(std::distance(items.begin(), iter));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL(), iter->url);
#endif

  // New tab is not created.
  EXPECT_EQ(2, tab_model()->count());
}

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
// Wallet sidebar tests with kBraveWalletSidePanel enabled: wallet item
// opens as a side panel instead of navigating a tab.
class SidebarBrowserTestWalletSidePanel : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWalletSidePanel() {
    wallet_feature_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletSidePanel);
  }

 protected:
  // Activates the wallet side panel via SidePanelUI (not ActivateItemAt, which
  // only updates the sidebar model). Returns the item index.
  std::optional<size_t> ActivateWalletPanel() {
    auto index = model()->GetIndexOf(SidebarItem::BuiltInItemType::kWallet);
    EXPECT_TRUE(index.has_value());

    controller()->ActivatePanelItem(SidebarItem::BuiltInItemType::kWallet);

    auto* panel_ui = browser()->GetFeatures().side_panel_ui();
    EXPECT_TRUE(panel_ui);
    EXPECT_TRUE(base::test::RunUntil([&]() {
      return panel_ui &&
             panel_ui->GetCurrentEntryId() == SidePanelEntryId::kWallet;
    }));
    return index;
  }

 private:
  base::test::ScopedFeatureList wallet_feature_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWalletSidePanel, WalletSidePanel) {
  const auto items = model()->GetAllSidebarItems();
  const auto wallet_item_iter =
      std::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                        &SidebarItem::built_in_item_type);
  ASSERT_NE(wallet_item_iter, items.cend());
  EXPECT_TRUE(wallet_item_iter->open_in_panel);

  const int initial_tab_count = tab_model()->count();

  ActivateWalletPanel();

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(panel_ui);
  auto current_entry = panel_ui->GetCurrentEntryId();
  ASSERT_TRUE(current_entry.has_value());
  EXPECT_EQ(SidePanelEntryId::kWallet, *current_entry);

  // Opening wallet as a side panel should not create a new tab.
  EXPECT_EQ(initial_tab_count, tab_model()->count());
}

// Wallet is contextual (per-tab): opening it on one tab should not keep it
// active after switching to another tab that has not opened it.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWalletSidePanel,
                       WalletSidePanelIsTabSpecific) {
  auto wallet_item_index = ActivateWalletPanel();
  ASSERT_TRUE(wallet_item_index.has_value());

  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(2, tab_model()->count());

  // New tab should not show the wallet panel as active.
  EXPECT_NE(model()->active_index(), wallet_item_index);

  // Returning to the original tab restores the contextual wallet panel.
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(model()->active_index(), wallet_item_index);
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(panel_ui);
  EXPECT_EQ(SidePanelEntryId::kWallet, panel_ui->GetCurrentEntryId());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PRE_LastlyUsedSidePanelItemTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Show(SidePanelEntryId::kBookmarks);

  // Wait till panel UI opens.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));

  // Check bookmarks panel is shown.
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, LastlyUsedSidePanelItemTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Toggle();

  // Wait till panel UI opens.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return GetSidePanel()->GetVisible(); }));

  // Check bookmarks item is opened after toggle.
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, DefaultEntryTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  panel_ui->Show(SidePanelEntryId::kBookmarks);

  // Wait till bookmark panel is activated.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  panel_ui->Close();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !panel_ui->GetCurrentEntryId().has_value(); }));

  // Remove bookmarks and check it's gone.
  SidebarServiceFactory::GetForProfile(browser()->GetProfile())
      ->RemoveItemAt(*bookmark_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Open panel w/o entry id.
  panel_ui->Toggle();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return panel_ui->GetCurrentEntryId().has_value(); }));
  // Check bookmark panel is not opened again as it's deleted item.
  EXPECT_NE(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemDragIndicatorCalcTest) {
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());
  gfx::Rect contents_view_rect = sidebar_items_contents_view->GetLocalBounds();
  views::View::ConvertRectToScreen(sidebar_items_contents_view,
                                   &contents_view_rect);
  gfx::Point screen_position = contents_view_rect.origin();
  screen_position.Offset(5, 0);

  // Any point from items contents view should have proper drag indicator index.
  for (int i = 0; i < contents_view_rect.height(); ++i) {
    gfx::Point simulated_mouse_drag_point = screen_position;
    simulated_mouse_drag_point.Offset(0, i);
    VerifyTargetDragIndicatorIndexCalc(simulated_mouse_drag_point);
  }
}

class SidebarBrowserWithSplitViewTest
    : public SidebarBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  SidebarBrowserWithSplitViewTest() = default;
  ~SidebarBrowserWithSplitViewTest() override = default;

  void SetUpOnMainThread() override {
    SidebarBrowserTest::SetUpOnMainThread();
    browser()->GetProfile()->GetPrefs()->SetBoolean(kWebViewRoundedCorners,
                                                    GetParam());
  }

  void NewSplitTab() {
    chrome::NewSplitTab(browser(), split_tabs::SplitTabLayout::kSideBySide,
                        split_tabs::SplitTabCreatedSource::kTabContextMenu);
  }

  // Use this when left split view is active.
  views::View* GetStartSplitContentsView() {
    return browser_view()
        ->GetBraveMultiContentsView()
        ->GetActiveContentsContainerView();
  }

  // Use this when left split view is active.
  views::View* GetEndSplitContentsView() {
    return browser_view()
        ->GetBraveMultiContentsView()
        ->GetInactiveContentsContainerView();
  }

  BraveBrowserView* browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

 private:
  display::test::TestScreen screen{/*create_display=*/true,
                                   /*register_screen=*/true};
};

IN_PROC_BROWSER_TEST_P(SidebarBrowserWithSplitViewTest,
                       ShowSidebarOnMouseOverTest) {
  auto scoped_mode = gfx::AnimationTestApi::SetRichAnimationRenderMode(
      gfx::Animation::RichAnimationRenderMode::FORCE_DISABLED);

  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  service->SetSidebarShowOption(
      SidebarService::ShowSidebarOption::kShowOnMouseOver);

  // To put sidebar right position after changing show option.
  browser_view()->DeprecatedLayoutImmediately();

  auto* browser_view = BraveBrowserView::GetBrowserViewForBrowser(browser());

  auto* prefs = browser()->GetProfile()->GetPrefs();
  auto* sidebar_container = GetSidebarContainerView();
  auto* screen = display::Screen::Get();

  auto contents_area_view_rect =
      browser_view->GetBoundingBoxInScreenForMouseOverHandling();
  EXPECT_EQ(browser_view->width(), contents_area_view_rect.width());

  // Check sidebar is not shown.
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Set mouse position inside the mouse hover area to check sidebar UI is shown
  // with that mouse position when sidebar is on right side.
  gfx::Point mouse_position = contents_area_view_rect.top_right();
  mouse_position.Offset(-2, 2);
  screen->SetCursorScreenPointForTesting(mouse_position);
  HandleBrowserWindowMouseEvent();
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Check when sidebar on left.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);

  // Hide sidebar.
  HideSidebar();
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Set mouse position inside the mouse hover area to check sidebar UI is shown
  // with that mouse position when sidebar is on left side.
  mouse_position = contents_area_view_rect.origin();
  mouse_position.Offset(2, 2);
  screen->SetCursorScreenPointForTesting(mouse_position);
  HandleBrowserWindowMouseEvent();
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Hide sidebar.
  HideSidebar();
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Check with the space between window border and contents.
  // We have that space with rounded corners.
  // When mouse moves into that space, sidebar should be visible.
  mouse_position = contents_area_view_rect.origin();
  screen->SetCursorScreenPointForTesting(mouse_position);
  HandleBrowserWindowMouseEvent();
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Hide sidebar.
  HideSidebar();
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Test with split view.
  // With sidebar on left, only left split view contents' left side hot
  // corner should trigger split view.
  NewSplitTab();
  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(2, tab_strip_model->count());

  // Make left split view as active to make secondary container put
  // at right side(end).
  tab_strip_model->ActivateTabAt(0);

  // Split containers are positioned by a scheduled layout. Flush it before
  // reading their bounds, otherwise the end container still has its initial
  // empty bounds at the contents area's origin.
  RunScheduledLayouts();

  auto* left_split_view = GetStartSplitContentsView();
  auto* right_split_view = GetEndSplitContentsView();
  ASSERT_FALSE(right_split_view->bounds().IsEmpty());

  // Check left split view's left hot corner handles.
  mouse_position = left_split_view->GetLocalBounds().origin();
  mouse_position.Offset(2, 2);
  views::View::ConvertPointToScreen(left_split_view, &mouse_position);
  screen->SetCursorScreenPointForTesting(mouse_position);
  HandleBrowserWindowMouseEvent();
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Hide sidebar.
  HideSidebar();
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // Check right split view's left hot corner doesn't handle.
  mouse_position = right_split_view->GetLocalBounds().origin();
  mouse_position.Offset(2, 2);
  views::View::ConvertPointToScreen(right_split_view, &mouse_position);
  screen->SetCursorScreenPointForTesting(mouse_position);
  HandleBrowserWindowMouseEvent();
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserWithSplitViewTest,
    ::testing::Bool());

class SidebarBrowserWithWebPanelTest
    : public SidebarBrowserTest,
      public testing::WithParamInterface<bool> {
 public:
  SidebarBrowserWithWebPanelTest() {
    if (GetParam()) {
      scoped_features_.InitAndEnableFeature(
          sidebar::features::kSidebarWebPanel);
    }
  }
  ~SidebarBrowserWithWebPanelTest() override = default;

  BraveMultiContentsView* GetBraveMultiContentsView() {
    return browser_view()->GetBraveMultiContentsView();
  }

  bool IsWebPanelEnabled() const {
    return base::FeatureList::IsEnabled(features::kSidebarWebPanel);
  }

  SidebarWebPanelController* web_panel_controller() {
    return controller()->GetWebPanelController();
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

IN_PROC_BROWSER_TEST_P(SidebarBrowserWithWebPanelTest, WebPanelTest) {
  auto contents_container_view_for_web_panel =
      GetBraveMultiContentsView()->contents_container_view_for_web_panel_;
  if (!IsWebPanelEnabled()) {
    EXPECT_FALSE(contents_container_view_for_web_panel);

    // Even web panel feature is disabled, sidebar could have web panel
    // type because it could be added when that feature enabled.
    // In this sitaution, web panel type should work like web type that
    // load its url in a tab.
    // Test it works in that way after adding web panel type.
    auto* sidebar_service =
        SidebarServiceFactory::GetForProfile(browser()->GetProfile());
    GURL item_url("http://foo.bar/");
    sidebar_service->AddItem(sidebar::SidebarItem::Create(
        item_url, u"title", SidebarItem::Type::kTypeWeb,
        SidebarItem::BuiltInItemType::kNone, /*open_in_panel*/ true));
    EXPECT_NE(tab_model()->GetActiveWebContents()->GetVisibleURL(), item_url);
    // Above item is added at last.
    controller()->ActivateItemAt(sidebar_service->items().size() - 1);
    EXPECT_EQ(tab_model()->GetActiveWebContents()->GetVisibleURL(), item_url);

    // Test toggle existing panel doesn't have any issue even web panel type
    // exists.
    auto* panel_ui = browser()->GetFeatures().side_panel_ui();
    panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return GetSidePanel()->GetVisible(); }));
    panel_ui->Close();
    ASSERT_TRUE(
        base::test::RunUntil([&]() { return !GetSidePanel()->GetVisible(); }));
    return;
  }

  EXPECT_TRUE(contents_container_view_for_web_panel);
  EXPECT_FALSE(contents_container_view_for_web_panel->GetVisible());
  EXPECT_GT(GetBraveMultiContentsView()->web_panel_width_, 0);
  EXPECT_EQ(GetBraveMultiContentsView()->GetWebPanelWidth(), 0);
  EXPECT_EQ(contents_container_view_for_web_panel->width(), 0);
  EXPECT_FALSE(GetBraveMultiContentsView()->web_panel_on_left_);
  EXPECT_EQ(
      GetBraveMultiContentsView()->width(),
      GetBraveMultiContentsView()->GetActiveContentsContainerView()->width());

  contents_container_view_for_web_panel->SetVisible(true);
  EXPECT_GT(GetBraveMultiContentsView()->GetWebPanelWidth(), 0);
  GetBraveMultiContentsView()->InvalidateLayout();
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return contents_container_view_for_web_panel->width() ==
           GetBraveMultiContentsView()->web_panel_width_;
  }));
  EXPECT_EQ(contents_container_view_for_web_panel->bounds().origin(),
            GetBraveMultiContentsView()
                ->GetActiveContentsContainerView()
                ->bounds()
                .top_right());

  GetBraveMultiContentsView()->SetWebPanelOnLeft(true);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return contents_container_view_for_web_panel->bounds().top_right() ==
           GetBraveMultiContentsView()
               ->GetActiveContentsContainerView()
               ->bounds()
               .origin();
  }));

  contents_container_view_for_web_panel->SetVisible(false);

  // To prevent item added bubble launching.
  auto* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  // Add two web panel items and check panel is shown by activating it.
  ASSERT_TRUE(
      ui_test_utils::NavigateToURL(browser(), GURL("https://brave.com/")));
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("https://basicattentiontoken.com/")));
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));
  controller()->AddItemWithCurrentTab();
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());

  // Activate web panel item and check panel gets visible.
  // We have 1 tabs now and will check another pinned tab is created for web
  // panel.
  auto* tab_strip_model = browser()->tab_strip_model();
  EXPECT_EQ(1, tab_strip_model->count());

  const int web_panel_item_index = model()->GetAllSidebarItems().size() - 1;
  EXPECT_TRUE(
      model()->GetAllSidebarItems()[web_panel_item_index].is_web_panel_type());
  EXPECT_TRUE(model()
                  ->GetAllSidebarItems()[web_panel_item_index - 1]
                  .is_web_panel_type());
  controller()->ActivateItemAt(web_panel_item_index - 1);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_TRUE(
      contents_container_view_for_web_panel->mini_toolbar()->GetVisible());

  // Activate another web panel and check panel is still visible.
  controller()->ActivateItemAt(web_panel_item_index);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_TRUE(
      contents_container_view_for_web_panel->mini_toolbar()->GetVisible());

  // Now we have another pinned tab for web panel at 0.
  EXPECT_EQ(2, tab_strip_model->count());
  auto* tab_for_web_panel = tab_strip_model->GetTabAtIndex(0);
  EXPECT_TRUE(tab_for_web_panel->IsPinned());
  EXPECT_FALSE(tab_for_web_panel->IsActivated());
  EXPECT_EQ(tab_for_web_panel->GetContents(),
            web_panel_controller()->panel_contents());

  // Toggle web panel item and check panel gets hidden.
  controller()->ActivateItemAt(web_panel_item_index);
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(1, tab_strip_model->count());

  // Open web panel.
  controller()->ActivateItemAt(web_panel_item_index);
  EXPECT_TRUE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(2, tab_strip_model->count());
  tab_for_web_panel = tab_strip_model->GetTabAtIndex(0);
  EXPECT_EQ(tab_for_web_panel->GetContents(),
            web_panel_controller()->panel_contents());

  // Web panel contents should be treated as active contents without
  // explicitly forcing focus (avoids macOS focus teardown issues).
  EXPECT_EQ(
      web_panel_controller()->panel_contents(),
      contents_container_view_for_web_panel->contents_view()->GetWebContents());

  // Check tab contents test with split view.
  // Create split view with tab at 1.
  tab_strip_model->ActivateTabAt(1);
  chrome::NewSplitTab(browser(), split_tabs::SplitTabLayout::kSideBySide,
                      split_tabs::SplitTabCreatedSource::kTabContextMenu);
  EXPECT_EQ(2, tab_strip_model->active_index());
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(1)->IsSplit());
  EXPECT_TRUE(tab_strip_model->GetTabAtIndex(2)->IsSplit());
  EXPECT_FALSE(GetBraveMultiContentsView()->is_web_panel_active_);

  // Cache current inactive split tab contents and check it's not changed for
  // inactive contents view when web panel is activated. Only active contents
  // view is changed to panel's contents as
  // BraveMultiContentsView::GetActiveContentsView() will give panel's contents.
  auto* inactive_split_tab_contents =
      GetBraveMultiContentsView()->GetInactiveContentsView()->GetWebContents();
  tab_strip_model->ActivateTabAt(0);
  EXPECT_TRUE(GetBraveMultiContentsView()->is_web_panel_active_);
  EXPECT_EQ(0, tab_strip_model->active_index());
  EXPECT_EQ(
      web_panel_controller()->panel_contents(),
      GetBraveMultiContentsView()->GetActiveContentsView()->GetWebContents());
  EXPECT_EQ(
      inactive_split_tab_contents,
      GetBraveMultiContentsView()->GetInactiveContentsView()->GetWebContents());

  // Check web panel is closed by closing its pinned tab.
  tab_for_web_panel->Close();
  EXPECT_FALSE(web_panel_controller()->panel_contents());
  EXPECT_FALSE(GetBraveMultiContentsView()->IsWebPanelVisible());
  EXPECT_EQ(2, tab_strip_model->count());
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserWithWebPanelTest,
    ::testing::Bool());

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, HideSidebarUITest) {
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto* sidebar_container = GetSidebarContainerView();

  // Set to on mouse over and check sidebar ui is not shown.
  service->SetSidebarShowOption(
      SidebarService::ShowSidebarOption::kShowOnMouseOver);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return sidebar_container->width() == 0; }));

  // Ask to show sidebar ui and check it's shown.
  ShowSidebar();
  const int target_control_view_width =
      GetSidebarControlView()->GetPreferredSize().width();
  EXPECT_GT(target_control_view_width, 0);
  WaitUntil(base::BindLambdaForTesting([&]() {
    return sidebar_container->width() == target_control_view_width;
  }));

  // Ask to hide sidebar ui and check it's not shown.
  HideSidebar();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return sidebar_container->width() == 0; }));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemAddedBubbleAnchorViewTest) {
  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());
  SetItemAddedBubbleLaunchedCallback(sidebar_items_contents_view);
  size_t lastly_added_item_index = 0;

  // Add item at last.
  item_added_bubble_anchor_ = nullptr;
  sidebar_service->AddItem(sidebar::SidebarItem::Create(
      GURL("http://foo.bar/"), u"title", SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, false));

  // Check item is added at last and check that bubble is anchored to
  // it properly.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !!item_added_bubble_anchor_; }));
  lastly_added_item_index = sidebar_items_contents_view->children().size() - 1;
  EXPECT_EQ(item_added_bubble_anchor_,
            sidebar_items_contents_view->children()[lastly_added_item_index]);

  // Add item at index 0.
  item_added_bubble_anchor_ = nullptr;
  sidebar_service->AddItemAtForTesting(
      sidebar::SidebarItem::Create(GURL("http://foo.bar/"), u"title",
                                   SidebarItem::Type::kTypeWeb,
                                   SidebarItem::BuiltInItemType::kNone, false),
      0);

  // Check item is added at first and check that bubble is anchored to
  // it properly.
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !!item_added_bubble_anchor_; }));
  lastly_added_item_index = 0;
  EXPECT_EQ(item_added_bubble_anchor_,
            sidebar_items_contents_view->children()[lastly_added_item_index]);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemActivatedScrollTest) {
  // To prevent item added bubble launching.
  auto* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto* scroll_view = GetSidebarItemsScrollView(controller());

  // Move bookmark item at zero index to make it hidden.
  sidebar_service->MoveItem(*bookmark_item_index, 0);
  bookmark_item_index = 0;
  AddItemsTillScrollable(scroll_view, sidebar_service);

  // Check bookmarks item is hidden.
  EXPECT_TRUE(NeedScrollForItemAt(*bookmark_item_index, scroll_view));

  // Open bookmark panel.
  browser()->GetFeatures().side_panel_ui()->Show(SidePanelEntryId::kBookmarks);

  // Wait till bookmarks item is visible.
  WaitUntil(base::BindLambdaForTesting([&]() {
    return !NeedScrollForItemAt(*bookmark_item_index, scroll_view);
  }));
  EXPECT_TRUE(controller()->IsActiveIndex(bookmark_item_index));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ItemAddedScrollTest) {
  // To prevent item added bubble launching.
  auto* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetInteger(sidebar::kSidebarItemAddedFeedbackBubbleShowCount, 3);

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto* scroll_view = GetSidebarItemsScrollView(controller());
  auto sidebar_items_contents_view = GetSidebarItemsContentsView(controller());

  AddItemsTillScrollable(scroll_view, sidebar_service);

  // Check first item is not visible in scroll view.
  EXPECT_TRUE(NeedScrollForItemAt(0, scroll_view));

  // After inserting item at index 0, it should be visible as sidebar
  // scrolls to make that new item visible. So last item becomes invisible.
  sidebar_service->AddItemAtForTesting(
      sidebar::SidebarItem::Create(GURL("https://abcd"), u"title",
                                   SidebarItem::Type::kTypeWeb,
                                   SidebarItem::BuiltInItemType::kNone, false),
      0);
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !NeedScrollForItemAt(0, scroll_view); }));

  int last_item_index = sidebar_items_contents_view->children().size() - 1;
  EXPECT_TRUE(NeedScrollForItemAt(last_item_index, scroll_view));

  // After inserting item at last, it should be visible as sidebar
  // scrolls to make that new item visible. So fisrt item becomes invisible.
  last_item_index++;
  sidebar_service->AddItem(sidebar::SidebarItem::Create(
      GURL("https://abcdefg"), u"title", SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, false));
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !NeedScrollForItemAt(last_item_index, scroll_view); }));
  EXPECT_TRUE(NeedScrollForItemAt(0, scroll_view));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PRE_PrefsMigrationTest) {
  // Prepare temporarily changed condition.
  auto* prefs = browser()->GetProfile()->GetPrefs();
  prefs->SetBoolean(sidebar::kSidebarAlignmentChangedTemporarily, true);
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PrefsMigrationTest) {
  // Check all prefs are changed to default.
  auto* prefs = browser()->GetProfile()->GetPrefs();
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
  EXPECT_TRUE(prefs->FindPreference(prefs::kSidePanelHorizontalAlignment)
                  ->IsDefaultValue());
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, DisabledItemsTest) {
  auto* guest_browser = CreateGuestBrowser();
  auto* controller = guest_browser->GetFeatures().sidebar_controller();
  auto* model = controller->model();
  for (const auto& item : model->GetAllSidebarItems()) {
    // Check disabled builtin items are not included in guest browser's items
    // list.
    if (item.is_built_in_type()) {
      EXPECT_FALSE(IsDisabledItemForGuest(item.built_in_item_type));
    }
  }

  auto* private_browser = CreateIncognitoBrowser(browser()->GetProfile());
  controller = private_browser->GetFeatures().sidebar_controller();
  model = controller->model();
  for (const auto& item : model->GetAllSidebarItems()) {
    // Check disabled builtin items are not included in private browser's items
    // list.
    if (item.is_built_in_type()) {
      EXPECT_FALSE(IsDisabledItemForPrivate(item.built_in_item_type));
    }
  }
}

class MockSidebarModelObserver : public SidebarModel::Observer {
 public:
  MockSidebarModelObserver() = default;
  ~MockSidebarModelObserver() override = default;

  MOCK_METHOD(void,
              OnItemAdded,
              (const SidebarItem& item, size_t index, bool user_gesture),
              (override));
  MOCK_METHOD(void,
              OnItemMoved,
              (const SidebarItem& item, size_t from, size_t to),
              (override));
  MOCK_METHOD(void, OnItemRemoved, (size_t index), (override));
  MOCK_METHOD(void,
              OnActiveIndexChanged,
              (std::optional<size_t> old_index,
               std::optional<size_t> new_index),
              (override));
  MOCK_METHOD(void,
              OnItemUpdated,
              (const SidebarItem& item, const SidebarItemUpdate& update),
              (override));
  MOCK_METHOD(void,
              OnFaviconUpdatedForItem,
              (const SidebarItem& item, const gfx::ImageSkia& image),
              (override));
};

#if BUILDFLAG(ENABLE_AI_CHAT)
class SidebarBrowserTestWithkSidebarShowAlwaysOnStable
    : public testing::WithParamInterface<bool>,
      public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithkSidebarShowAlwaysOnStable() {
    if (GetParam()) {
      feature_list_.InitAndEnableFeatureWithParameters(
          sidebar::features::kSidebarShowAlwaysOnStable,
          {{"open_one_shot_leo_panel", "true"}});
    } else {
      feature_list_.InitAndEnableFeature(
          sidebar::features::kSidebarShowAlwaysOnStable);
    }
  }
  ~SidebarBrowserTestWithkSidebarShowAlwaysOnStable() override = default;

  void SetUp() override { SidebarBrowserTest::SetUp(); }

  void TearDown() override { SidebarBrowserTest::TearDown(); }

  // For skipping SidebarBrowserTest's PreRunTestOnMainThread
  // as show option is set explicitely there.
  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    SidebarBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kDontShowSidebarOnNonStable);
    command_line->AppendSwitch(switches::kForceFirstRun);
  }

  base::test::ScopedFeatureList feature_list_;
  testing::NiceMock<MockSidebarModelObserver> observer_;
  base::ScopedObservation<SidebarModel, SidebarModel::Observer> observation_{
      &observer_};
};

IN_PROC_BROWSER_TEST_P(SidebarBrowserTestWithkSidebarShowAlwaysOnStable,
                       SidebarShowAlwaysTest) {
  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  EXPECT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            sidebar_service->GetSidebarShowOption());

  observation_.Observe(model());

  // Check one shot Leo panel is opened or not based on test parameter.
  if (GetParam()) {
    // If Leo panel is opened, panel active index is changed.
    EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_))
        .Times(1);
  } else {
    EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_))
        .Times(0);
  }

  // Need to make sure template url service is loaded before testing one-shot
  // leo panel open. As we don't want to show one-shot leo panel open for
  // SERP, SidebarTabHelper uses template url service for checking current
  // url is SERP page or not. If template url service is not loaded, it just
  // does early return w/o opening leo panel.
  // See SidebarTabHelper::PrimaryPageChanged() for more details.
  auto* service =
      TemplateURLServiceFactory::GetForProfile(browser()->GetProfile());
  search_test_utils::WaitForTemplateURLServiceToLoad(service);

  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://www.brave.com/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  if (GetParam()) {
    // Wait till browser has active panel.
    WaitUntil(base::BindLambdaForTesting(
        [&]() { return !!panel_ui->GetCurrentEntryId(); }));

    EXPECT_EQ(SidePanelEntryId::kChatUI, panel_ui->GetCurrentEntryId());
  }
  testing::Mock::VerifyAndClearExpectations(&observer_);

  panel_ui->Close();
  WaitUntil(base::BindLambdaForTesting(
      [&]() { return !panel_ui->IsSidePanelShowing(); }));

  // Check one shot panel is not opened anymore.
  EXPECT_CALL(observer_, OnActiveIndexChanged(testing::_, testing::_)).Times(0);
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("https://www.brave.com/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  EXPECT_FALSE(panel_ui->IsSidePanelShowing());
  testing::Mock::VerifyAndClearExpectations(&observer_);

  observation_.Reset();
}

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    SidebarBrowserTestWithkSidebarShowAlwaysOnStable,
    ::testing::Bool());
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_PLAYLIST)
class SidebarBrowserTestWithPlaylist : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithPlaylist() {
    feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~SidebarBrowserTestWithPlaylist() override = default;

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithPlaylist, Incognito) {
  // There should be no crash with incognito.
  auto* private_browser = CreateIncognitoBrowser(browser()->GetProfile());
  ASSERT_TRUE(private_browser);

  auto* sidebar_service =
      SidebarServiceFactory::GetForProfile(private_browser->GetProfile());
  const auto& items = sidebar_service->items();
  auto iter = std::ranges::find_if(items, [](const auto& item) {
    return item.type == SidebarItem::Type::kTypeBuiltIn &&
           item.built_in_item_type == SidebarItem::BuiltInItemType::kPlaylist;
  });

  // Check playlist item is not included in private window.
  EXPECT_EQ(iter, items.end());

  // Try Adding an item
  sidebar_service->AddItem(sidebar::SidebarItem::Create(
      GURL("http://foo.bar/"), u"title", SidebarItem::Type::kTypeWeb,
      SidebarItem::BuiltInItemType::kNone, false));

  // Try moving an item
  sidebar_service->MoveItem(sidebar_service->items().size() - 1, 0);

  // Try Remove an item
  sidebar_service->RemoveItemAt(0);
}
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

#if BUILDFLAG(ENABLE_AI_CHAT)

// Fixture for the tab-scoped (non-global) AI Chat sidebar panel. The
// TabSpecific tests below rely on the AI Chat sidebar item being a tab-specific
// entry, which only happens when kAIChatGlobalSidePanelEverywhere is disabled —
// with the global side panel feature on, AI Chat is registered as a global
// entry.
class SidebarBrowserTestWithTabSpecificAIChat : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithTabSpecificAIChat() {
    feature_list_.InitWithFeatures(
        /*enabled_features=*/{ai_chat::features::kAIChat},
        /*disabled_features=*/
        {ai_chat::features::kAIChatGlobalSidePanelEverywhere});
  }
  ~SidebarBrowserTestWithTabSpecificAIChat() override = default;

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithTabSpecificAIChat,
                       TabSpecificPanel) {
  // Collect item indexes for test
  constexpr auto kGlobalItemType = SidebarItem::BuiltInItemType::kBookmarks;
  constexpr auto kTabSpecificItemType = SidebarItem::BuiltInItemType::kChatUI;
  auto global_item_index = model()->GetIndexOf(kGlobalItemType);
  ASSERT_TRUE(global_item_index.has_value());
  auto tab_specific_item_index = model()->GetIndexOf(kTabSpecificItemType);
  ASSERT_TRUE(tab_specific_item_index.has_value());

  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->count(), 2);

  // Open contextual panel from Tab 0.
  tab_model()->ActivateTabAt(0);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  // Delete Tab 0 and check model doesn't have active index.
  tab_model()->DetachAndDeleteWebContentsAt(0);
  EXPECT_FALSE(!!model()->active_index());
  ASSERT_EQ(tab_model()->count(), 1);

  // Create two more tab for test below.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->count(), 3);

  // Open a "global" panel from Tab 0
  tab_model()->ActivateTabAt(0);
  SimulateSidebarItemClickAt(global_item_index.value());
  // Open a "tab specific" panel from Tab 1
  tab_model()->ActivateTabAt(1);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);
  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(model()->active_index(), global_item_index);
  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(model()->active_index(), global_item_index);
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithTabSpecificAIChat,
                       TabSpecificPanelAndUnManagedPanel) {
  // Collect item indexes for test and remove global item.
  constexpr auto kGlobalItemType = SidebarItem::BuiltInItemType::kBookmarks;
  constexpr auto kTabSpecificItemType = SidebarItem::BuiltInItemType::kChatUI;
  auto global_item_index = model()->GetIndexOf(kGlobalItemType);
  ASSERT_TRUE(global_item_index.has_value());
  SidebarServiceFactory::GetForProfile(browser()->GetProfile())
      ->RemoveItemAt(*global_item_index);
  EXPECT_FALSE(!!model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  auto tab_specific_item_index = model()->GetIndexOf(kTabSpecificItemType);
  ASSERT_TRUE(tab_specific_item_index.has_value());
  // Open 2 more tabs
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->count(), 3);

  // Open a unmanaged "global" panel from Tab 0
  tab_model()->ActivateTabAt(0);
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  // Wait till sidebar show ends.
  WaitUntil(base::BindLambdaForTesting([&]() {
    return GetSidePanel()->width() >=
           SidePanelEntry::kSidePanelDefaultContentWidth;
  }));
  // Unmanaged entry becomes managed when its panel is shown
  // and it becomes active item.
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Open a "tab specific" panel from Tab 1
  tab_model()->ActivateTabAt(1);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  EXPECT_EQ(SidePanelEntryId::kChatUI, panel_ui->GetCurrentEntryId());
  EXPECT_TRUE(GetSidePanel()->GetVisible());
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(SidePanelEntryId::kBookmarks, panel_ui->GetCurrentEntryId());
  EXPECT_TRUE(model()->active_index().has_value());
  EXPECT_EQ(model()->active_index(),
            model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks));

  // Check tab specific panel item is still activated after moving to another
  // browser.
  tab_model()->ActivateTabAt(1);
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);

  auto* browser2 = CreateBrowser(browser()->GetProfile());
  auto* browser2_model = browser2->GetFeatures().sidebar_controller()->model();
  auto* browser2_tab_model = browser2->tab_strip_model();

  auto detached_tab = tab_model()->DetachTabAtForInsertion(1);
  browser2_tab_model->AppendTab(std::move(detached_tab), /* foreground */ true);
  EXPECT_EQ(browser2_model->active_index(),
            browser2_model->GetIndexOf(SidebarItem::BuiltInItemType::kChatUI));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithTabSpecificAIChat,
                       TabSpecificPanelIdxChange) {
  // Collect item indexes for test
  constexpr auto kGlobalItemType = SidebarItem::BuiltInItemType::kBookmarks;
  constexpr auto kTabSpecificItemType = SidebarItem::BuiltInItemType::kChatUI;
  auto global_item_index = model()->GetIndexOf(kGlobalItemType);
  ASSERT_TRUE(global_item_index.has_value());
  auto tab_specific_item_index = model()->GetIndexOf(kTabSpecificItemType);
  ASSERT_TRUE(tab_specific_item_index.has_value());
  // Open 2 more tabs
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("chrome://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  ASSERT_EQ(tab_model()->count(), 3);
  // Open a "global" panel from Tab 0
  tab_model()->ActivateTabAt(0);
  SimulateSidebarItemClickAt(global_item_index.value());
  // Open a "tab specific" panel from Tab 1
  tab_model()->ActivateTabAt(1);
  SimulateSidebarItemClickAt(tab_specific_item_index.value());
  // Move global item
  size_t new_global_item_index = (global_item_index > 0u) ? 0u : 1u;
  SidebarServiceFactory::GetForProfile(browser()->GetProfile())
      ->MoveItem(global_item_index.value(), new_global_item_index);
  tab_specific_item_index = model()->GetIndexOf(kTabSpecificItemType);
  // Tab Specific panel should be open when Tab 1 is active
  EXPECT_EQ(model()->active_index(), tab_specific_item_index);
  // Global panel should be open when Tab 0 is active
  tab_model()->ActivateTabAt(0);
  EXPECT_EQ(model()->active_index(), new_global_item_index);
  // Global panel should be open when Tab 2 is active
  tab_model()->ActivateTabAt(2);
  EXPECT_EQ(model()->active_index(), new_global_item_index);
}

#endif  // BUILDFLAG(ENABLE_AI_CHAT)

// Growser-151: the default the product actually ships. Upstream starts the
// sidebar on the right and every test around this one now sets the alignment
// it needs, so without this nothing states the default at all - and a lost
// default is exactly the kind of change that reaches a user unannounced.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, StartsOnTheLeftByDefault) {
  EXPECT_TRUE(IsSidebarUIOnLeft());
  EXPECT_FALSE(browser()->GetProfile()->GetPrefs()->GetBoolean(
      prefs::kSidePanelHorizontalAlignment));
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarRightSideTest) {
  // Growser-151: the right is UPSTREAM's default. Growser starts the sidebar
  // on the left (brave_profile_prefs.cc), and this test is about the
  // right-hand layout - so it asks for that layout instead of assuming it.
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      prefs::kSidePanelHorizontalAlignment, true);
  ASSERT_FALSE(IsSidebarUIOnLeft());

  brave::ToggleVerticalTabStrip(browser());
  ASSERT_TRUE(VerticalTabController::FromBrowser(browser())
                  ->ShouldShowBraveVerticalTabs());

  auto* prefs = browser()->GetProfile()->GetPrefs();
  auto* vertical_tabs_container = GetVerticalTabsContainer();
  views::View* sidebar_container = GetSidebarContainerView();

  // Check if vertical tabs is located at first and sidebar is located on the
  // right side.
  EXPECT_LT(vertical_tabs_container->GetBoundsInScreen().x(),
            sidebar_container->GetBoundsInScreen().x());

  // Changed to sidebar on left side.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  EXPECT_TRUE(IsSidebarUIOnLeft());

  int expected_sidebar_x = vertical_tabs_container->GetBoundsInScreen().right();
  if (BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser())) {
    expected_sidebar_x += 1;
  }

  // Check if vertical tabs is located first and sidebar is following it.
  EXPECT_EQ(sidebar_container->GetBoundsInScreen().x(), expected_sidebar_x);

  // Check sidebar position option is synced between normal and private window.
  auto* private_browser = CreateIncognitoBrowser(browser()->GetProfile());
  auto* private_prefs = private_browser->GetProfile()->GetPrefs();
  EXPECT_EQ(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment),
            private_prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  EXPECT_FALSE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
  private_prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment));
}

// Three fixes work together:
//   - SetMirrored(false) on SidebarControlView prevents
//     the Views framework from flipping internal layout in RTL.
//   - SetFlipCanvasOnPaintForRTLUI(false) on buttons prevents icon flipping.
//   - BraveBrowserViewTabbedLayoutImpl flips the alignment pref into a
//     leading-edge boolean in stored coordinates (IsSideBarLeading), so the
//     sidebar renders on the visual side the user chose in RTL.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarLayoutInRTLTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* sidebar_container = GetSidebarContainerView();
  auto* panel = GetSidePanel();
  panel->DisableAnimationsForTesting();
  auto* contents_view = browser_view->contents_container();
  auto* prefs = browser()->GetProfile()->GetPrefs();

  // --- Right-aligned sidebar ---
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  ASSERT_FALSE(IsSidebarUIOnLeft());

  // Needs invalidation to apply RTL mode layout.
  base::i18n::ScopedRTLForTesting scoped_rtl(/*rtl=*/true);
  browser_view->InvalidateLayout();
  RunScheduledLayouts();

  // Mirrored bounds are the visual positions in RTL: the sidebar must stay on
  // the visual right, matching the user's alignment pref.
  EXPECT_LE(contents_view->GetMirroredBounds().right(),
            sidebar_container->GetMirroredBounds().x());

  // Open the side panel to verify panel/control positioning.
  chrome::BrowserCommandController::From(browser())->ExecuteCommand(
      IDC_TOGGLE_SIDEBAR);
  RunScheduledLayouts();
  ASSERT_TRUE(base::test::RunUntil([&]() {
    // panel can have border insets.
    return panel->width() >= SidePanelEntry::kSidePanelDefaultContentWidth;
  }));

  // Mirrored bounds are the visual positions in RTL:
  // panel must stay on the visual left.
  EXPECT_LE(panel->GetMirroredBounds().right(),
            sidebar_container->GetMirroredBounds().x());

  // --- Left-aligned sidebar in RTL mode ---
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  ASSERT_TRUE(IsSidebarUIOnLeft());
  RunScheduledLayouts();

  // Mirrored bounds are the visual positions in RTL:
  // panel must stay on the visual right.
  EXPECT_LE(sidebar_container->GetMirroredBounds().right(),
            panel->GetMirroredBounds().x());

  // Mirrored bounds are the visual positions in RTL: the sidebar must stay on
  // the visual left, matching the user's alignment pref.
  EXPECT_LE(sidebar_container->GetMirroredBounds().right(),
            contents_view->GetMirroredBounds().x());
}

using views::ShapeContextTokensOverride::kRoundedCornersBorderRadius;
using views::ShapeContextTokensOverride::
    kRoundedCornersBorderRadiusAtWindowCorner;

// The upstream side panel is a direct child of browser_view, positioned
// by CalculateSideBarLayout.  Verify that when the panel is open it sits
// between the contents container and the sidebar control, NOT outside it.
// Covers: right-side, left-side, and VT+sidebar on the same side (regression
// for the vtab_width gap bug fixed in ComputeAdjustedPanelBounds).
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PanelPositionTest) {
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* panel = browser_view->side_panel();
  panel->DisableAnimationsForTesting();
  SidebarContainerView* sidebar = GetSidebarContainerView();
  auto* prefs = browser()->GetProfile()->GetPrefs();
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();

  browser()->GetFeatures().side_panel_ui()->Toggle();
  RunScheduledLayouts();
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return panel_ui->IsSidePanelShowing(); }));

  ASSERT_TRUE(panel->GetVisible());
  ASSERT_TRUE(sidebar->IsSidebarVisible());

  // --- Sidebar on right. Growser-151: growser's default is the left, so the
  // right-hand half of this test sets the alignment it measures.
  browser()->GetProfile()->GetPrefs()->SetBoolean(
      prefs::kSidePanelHorizontalAlignment, true);
  RunScheduledLayouts();
  ASSERT_FALSE(sidebar->sidebar_on_left());

  auto* contents = browser_view->contents_container();

  // Panel sits immediately left of the sidebar control:
  //   [contents] [panel] [sidebar_control]
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return panel->bounds().right() == sidebar->bounds().x(); }));

  // Panel top must align with the contents container — the upstream layout
  // offsets the panel -1px to overlap the toolbar separator; Brave removes
  // that offset so the separator is fully visible.
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return panel->bounds().y() == contents->bounds().y(); }));

  // --- Sidebar on left (kSidePanelHorizontalAlignment = false)
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  RunScheduledLayouts();

  ASSERT_TRUE(sidebar->sidebar_on_left());

  // Panel sits immediately right of the sidebar control:
  //   [sidebar_control] [panel] [contents]
  EXPECT_EQ(sidebar->bounds().right(), panel->bounds().x())
      << "sidebar=" << sidebar->bounds().ToString()
      << " panel=" << panel->bounds().ToString();

  EXPECT_EQ(panel->bounds().y(), contents->bounds().y())
      << "panel y=" << panel->bounds().y()
      << " contents y=" << contents->bounds().y();

  // --- VT and sidebar on the same left side (VT left by default, sidebar
  // left). Before the fix, the panel was misplaced by vtab_width, leaving a gap
  // between the panel and the contents container.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  brave::ToggleVerticalTabStrip(browser());
  ASSERT_TRUE(VerticalTabController::FromBrowser(browser())
                  ->ShouldShowBraveVerticalTabs());
  // VT defaults to left (kVerticalTabsOnRight = false).
  ASSERT_FALSE(
      VerticalTabController::FromBrowser(browser())->IsVerticalTabOnRight());
  RunScheduledLayouts();

  ASSERT_TRUE(sidebar->sidebar_on_left());

  // Panel sits immediately right of the sidebar control; no gap to contents.
  //   [VT] [sidebar_control] [panel] [contents]
  EXPECT_EQ(sidebar->bounds().right(), panel->bounds().x())
      << "sidebar=" << sidebar->bounds().ToString()
      << " panel=" << panel->bounds().ToString();
  EXPECT_EQ(panel->bounds().y(), contents->bounds().y())
      << "panel y=" << panel->bounds().y()
      << " contents y=" << contents->bounds().y();

  // --- VT and sidebar on the same right side (VT right, sidebar right).
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  prefs->SetBoolean(brave_tabs::kVerticalTabsOnRight, true);
  ASSERT_TRUE(
      VerticalTabController::FromBrowser(browser())->IsVerticalTabOnRight());
  RunScheduledLayouts();

  ASSERT_FALSE(sidebar->sidebar_on_left());

  // Panel sits immediately left of the sidebar control; no gap to contents.
  //   [contents] [panel] [sidebar_control] [VT]
  EXPECT_EQ(panel->bounds().right(), sidebar->bounds().x())
      << "panel=" << panel->bounds().ToString()
      << " sidebar=" << sidebar->bounds().ToString();
  EXPECT_EQ(panel->bounds().y(), contents->bounds().y())
      << "panel y=" << panel->bounds().y()
      << " contents y=" << contents->bounds().y();
}

// Verify that the sidebar item active state in SidebarModel is updated:
// - When clicking a panel item via the sidebar UI.
// - When the side panel is opened or closed via the side panel UI directly
//   (e.g. toolbar toggle button), which bypasses SidebarController.
//   SidebarContainerView does not monitor panel show/hide events, so
//   BraveSidePanelCoordinator updates the active state in Show() and Close().
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarActiveItemStateSync) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->DisableAnimationsForTesting();

  const auto bookmark_item_index =
      model()->GetIndexOf(SidebarItem::BuiltInItemType::kBookmarks);
  ASSERT_TRUE(bookmark_item_index.has_value());

  // Initially no item is active.
  EXPECT_FALSE(model()->active_index());

  // Clicking a panel item via the sidebar UI activates it in the model.
  SimulateSidebarItemClickAt(*bookmark_item_index);
  EXPECT_EQ(model()->active_index(), bookmark_item_index);

  // Deactivate by closing the panel.
  panel_ui->Close();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model()->active_index().has_value(); }));

  // Opening the side panel via the panel UI (e.g. toolbar toggle button path)
  // also activates the corresponding sidebar item in the model.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  // Closing the side panel via the panel UI deactivates the item in the model.
  panel_ui->Close();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model()->active_index().has_value(); }));

  // Toggling the panel open (as the toolbar button does) activates the
  // last-used sidebar item in the model.
  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return controller()->IsActiveIndex(bookmark_item_index); }));

  // Wait for the panel to be fully shown before toggling closed, so that
  // BraveSidePanelCoordinator::Toggle() sees IsSidePanelShowing() == true
  // and takes the close branch instead of the show branch.
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return panel_ui->IsSidePanelShowing(); }));

  // Toggling the panel closed deactivates the item in the model.
  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil(
      [&]() { return !model()->active_index().has_value(); }));
}

// Verify the Brave-styled side panel header is attached for reading list and
// bookmarks and absent for other entries.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, BraveSidePanelHeaderTest) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* side_panel = browser_view->side_panel();
  side_panel->DisableAnimationsForTesting();

  // Reading list: Brave header attached.
  panel_ui->Show(SidePanelEntryId::kReadingList);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kReadingList));
  }));
  EXPECT_NE(nullptr, side_panel->GetHeaderView<BraveSidePanelHeader>())
      << "BraveSidePanelHeader should be attached for the reading list panel";

  // Bookmarks: Brave header attached.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kBookmarks));
  }));
  EXPECT_NE(nullptr, side_panel->GetHeaderView<BraveSidePanelHeader>())
      << "BraveSidePanelHeader should be attached for the bookmarks panel";

  // CustomizeChrome: no Brave header (the previous one must be cleared when
  // switching to an entry that doesn't request a Brave header).
  panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kCustomizeChrome));
  }));
  EXPECT_EQ(nullptr, side_panel->GetHeaderView<BraveSidePanelHeader>())
      << "BraveSidePanelHeader should not be attached for CustomizeChrome";
}

namespace {

// Asserts layer-backed content children carry the expected corner radii.
// Children without compositor layers (the typical WebView holder path) are
// silently skipped; their corners can't be observed via public API.
void ExpectContentChildLayerCorners(SidePanel* panel,
                                    const gfx::RoundedCornersF& expected,
                                    const base::Location& loc = FROM_HERE) {
  SCOPED_TRACE(loc.ToString());
  for (const auto child : panel->GetContentParentView()->children()) {
    if (child->layer()) {
      EXPECT_EQ(expected, child->layer()->rounded_corner_radii());
    }
  }
}

}  // namespace

// Verify that content corner radii stay correct across three triggers:
//   (a) panel type change (header ↔ no-header entry),
//   (b) rounded-corners pref toggle while the panel is open,
//   (c) panel reopened after the pref changed while it was closed.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest,
                       PanelContentCornersUpdateOnStateChange) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* side_panel =
      BrowserView::GetBrowserViewForBrowser(browser())->side_panel();
  auto* prefs = browser()->GetProfile()->GetPrefs();
  side_panel->DisableAnimationsForTesting();
  prefs->SetBoolean(kWebViewRoundedCorners, true);

  const int r = views::LayoutProvider::Get()->GetCornerRadiusMetric(
      kRoundedCornersBorderRadius);
  const gfx::RoundedCornersF flat_top(0, 0, r, r);
  const gfx::RoundedCornersF all_round(r);
  const gfx::RoundedCornersF none;

  auto wait_for_entry = [&](SidePanelEntryId id,
                            const base::Location& loc = FROM_HERE) {
    SCOPED_TRACE(loc.ToString());
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return panel_ui->IsSidePanelEntryShowing(SidePanelEntry::Key(id));
    }));
  };

  // (a) Panel type change -----------------------------------------------

  // Bookmarks has a Brave header → top corners must be flat.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  wait_for_entry(SidePanelEntryId::kBookmarks);
  EXPECT_NE(nullptr, side_panel->GetHeaderView<views::View>());
  EXPECT_EQ(flat_top, brave::GetPanelContentsRoundedCorners(browser_view()));
  ExpectContentChildLayerCorners(side_panel, flat_top);

  // CustomizeChrome has no Brave header → all corners must be round.
  panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
  wait_for_entry(SidePanelEntryId::kCustomizeChrome);
  EXPECT_EQ(nullptr, side_panel->GetHeaderView<views::View>());
  EXPECT_EQ(all_round, brave::GetPanelContentsRoundedCorners(browser_view()));
  ExpectContentChildLayerCorners(side_panel, all_round);

  // Back to bookmarks → flat top again.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  wait_for_entry(SidePanelEntryId::kBookmarks);
  EXPECT_NE(nullptr, side_panel->GetHeaderView<views::View>());
  EXPECT_EQ(flat_top, brave::GetPanelContentsRoundedCorners(browser_view()));
  ExpectContentChildLayerCorners(side_panel, flat_top);

  // (b) Pref change while panel is open ---------------------------------

  // Pref OFF: no corners regardless of header (UpdateBorder() path).
  prefs->SetBoolean(kWebViewRoundedCorners, false);
  EXPECT_EQ(none, brave::GetPanelContentsRoundedCorners(browser_view()));
  ExpectContentChildLayerCorners(side_panel, none);

  // Pref ON again: flat top (bookmarks has header).
  prefs->SetBoolean(kWebViewRoundedCorners, true);
  EXPECT_EQ(flat_top, brave::GetPanelContentsRoundedCorners(browser_view()));
  ExpectContentChildLayerCorners(side_panel, flat_top);

  // (c) Panel reopened after pref changed while closed ------------------

  panel_ui->Close();
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !side_panel->GetVisible(); }));

  // Toggle pref while the panel is hidden.
  prefs->SetBoolean(kWebViewRoundedCorners, false);

  // Reopen — Open() must re-apply corners with the new pref value.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  wait_for_entry(SidePanelEntryId::kBookmarks);
  EXPECT_EQ(none, brave::GetPanelContentsRoundedCorners(browser_view()));
  ExpectContentChildLayerCorners(side_panel, none);
}

// Verify that toggling the sidebar UI's visibility while a panel is open
// re-applies the panel's content corners. SidebarContainerView runs a
// visibility-changed callback (wired to BraveBrowserView::UpdateBorder() on the
// panel), so the inner bottom corner flips between the regular radius (sidebar
// visible) and the window-corner radius (sidebar hidden, panel flush with the
// window edge) without any other trigger. The inner bottom corner is the
// lower-right for a right-aligned panel and the lower-left for a left-aligned
// panel, so the cycle is exercised under both alignments. Each step checks both
// the helper's computed corners and the corners actually applied to the content
// child layers.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest,
                       PanelContentCornersFollowSidebarVisibility) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* side_panel = browser_view()->side_panel();
  auto* prefs = browser()->GetProfile()->GetPrefs();
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  side_panel->DisableAnimationsForTesting();
  prefs->SetBoolean(kWebViewRoundedCorners, true);

  const int r = views::LayoutProvider::Get()->GetCornerRadiusMetric(
      kRoundedCornersBorderRadius);
  const int rw = views::LayoutProvider::Get()->GetCornerRadiusMetric(
      kRoundedCornersBorderRadiusAtWindowCorner);

  // Open a panel with a Brave header, so the top corners are flat and only the
  // inner bottom corner varies with sidebar visibility.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kBookmarks));
  }));

  // Asserts both the computed corners and the corners applied to content
  // layers.
  auto expect_corners = [&](const gfx::RoundedCornersF& expected,
                            const base::Location& loc = FROM_HERE) {
    SCOPED_TRACE(loc.ToString());
    EXPECT_EQ(expected, brave::GetPanelContentsRoundedCorners(browser_view()));
    ExpectContentChildLayerCorners(side_panel, expected);
  };

  // Sidebar visible: the inner bottom corner uses the regular radius regardless
  // of alignment.
  const gfx::RoundedCornersF visible(0, 0, r, r);
  struct Case {
    bool right_aligned;
    gfx::RoundedCornersF hidden;  // inner bottom corner is the window corner.
  };
  const Case cases[] = {
      {true, gfx::RoundedCornersF(0, 0, rw, r)},   // right → lower-right.
      {false, gfx::RoundedCornersF(0, 0, r, rw)},  // left  → lower-left.
  };

  for (const auto& c : cases) {
    SCOPED_TRACE(c.right_aligned ? "right-aligned" : "left-aligned");
    prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, c.right_aligned);

    // Sidebar visible (kShowAlways): regular radius.
    service->SetSidebarShowOption(
        SidebarService::ShowSidebarOption::kShowAlways);
    ASSERT_TRUE(base::test::RunUntil(
        [&]() { return browser_view()->IsSidebarVisible(); }));
    expect_corners(visible);

    // Hide the sidebar UI: the callback must drive UpdateBorder() so the inner
    // bottom corner becomes the window-corner radius.
    service->SetSidebarShowOption(
        SidebarService::ShowSidebarOption::kShowNever);
    ASSERT_TRUE(base::test::RunUntil(
        [&]() { return !browser_view()->IsSidebarVisible(); }));
    expect_corners(c.hidden);

    // Show the sidebar UI again: back to the regular radius.
    service->SetSidebarShowOption(
        SidebarService::ShowSidebarOption::kShowAlways);
    ASSERT_TRUE(base::test::RunUntil(
        [&]() { return browser_view()->IsSidebarVisible(); }));
    expect_corners(visible);
  }
}

// Verify that the resize area is positioned correctly for both border states.
// In both cases the strip starts at the panel's outer edge with width
// kResizeStripWidth. Border state only affects z-order: without rounded
// corners the strip is reordered to the top so it wins the hit-test over
// the overlapping content edge.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest,
                       PanelResizeAreaPositionMatchesBorderState) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* side_panel = browser_view->side_panel();
  side_panel->DisableAnimationsForTesting();
  auto* prefs = browser()->GetProfile()->GetPrefs();

  // Growser-151: growser's default is the left; this test measures the
  // right-hand layout, so it sets it.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);

  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));

  auto* resize_area = side_panel->resize_area_for_testing();
  ASSERT_TRUE(resize_area);

  // Verifies resize strip position, width, z-order, and panel insets for the
  // given border mode. Panel is on the right in LTR, so the content-facing
  // edge is left and the outer edge is right.
  auto verify_state = [&](bool rounded_corners,
                          const base::Location& loc = FROM_HERE) {
    SCOPED_TRACE(loc.ToString());
    if (rounded_corners) {
      EXPECT_EQ(1, side_panel->GetInsets().left())
          << "rounded: 1px gap on the content-facing side, reserved for the "
             "outline (the content view otherwise owns its margin)";
      EXPECT_EQ(kRoundedCornersContentsViewMargin +
                    kRoundedCornersContentsOutlineThickness,
                side_panel->GetInsets().right())
          << "rounded: outer-side gap matches main content (margin + outline "
             "thickness)";
    } else {
      EXPECT_EQ(1, side_panel->GetInsets().left())
          << "plain: 1px separator on the content-facing side";
      EXPECT_EQ(0, side_panel->GetInsets().right())
          << "plain: no outer-side inset";
    }
    EXPECT_EQ(resize_area->GetBoundsInScreen().origin(),
              side_panel->GetBoundsInScreen().origin())
        << "resize strip should sit at the panel's outer edge";
    EXPECT_EQ(resize_area->width(),
              views::BraveSidePanelResizeArea::kResizeStripWidth)
        << "resize strip width should equal kResizeStripWidth";
    EXPECT_EQ(side_panel->GetIndexOf(resize_area),
              side_panel->children().size() - 1)
        << "resize strip should be the top-most child view";
  };

  prefs->SetBoolean(kWebViewRoundedCorners, true);
  RunScheduledLayouts();
  verify_state(true);

  prefs->SetBoolean(kWebViewRoundedCorners, false);
  RunScheduledLayouts();
  verify_state(false);

  // Switch to another panel that has brave panel header.
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kBookmarks));
  }));
  RunScheduledLayouts();
  verify_state(false);

  // Switch to another panel that doesn't have brave panel header.
  panel_ui->Show(SidePanelEntryId::kCustomizeChrome);
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kCustomizeChrome));
  }));
  RunScheduledLayouts();
  verify_state(false);
}

// Verify that the panel border insets follow the sidebar's horizontal
// alignment, in both LTR and RTL. Flipping kSidePanelHorizontalAlignment runs
// through BraveBrowserView::UpdateSideBarHorizontalAlignment(), and toggling
// kWebViewRoundedCorners runs through UpdateRoundedCornersUI(); both end up in
// SidePanel::UpdateBorder(). The content-facing edge owns the separator/margin
// and the outer edge owns the rounded-corner gap.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PanelBorderInsetsFollowAlignment) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  auto* side_panel = browser_view->side_panel();
  side_panel->DisableAnimationsForTesting();
  auto* prefs = browser()->GetProfile()->GetPrefs();

  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));

  // Border insets live in the panel's stored (pre-mirror) coordinate space, so
  // they track the leading edge rather than the visual side: the web contents
  // sit on the panel's high-X (right) stored side when the sidebar is leading,
  // and on the low-X (left) side when it is trailing. browser_view
  // mirror-paints the panel in RTL, so the separator still renders against the
  // contents visually. This mirrors SidePanel::UpdateBorder()'s own
  // is_sidebar_leading = (IsRightAligned() == IsRTL()) logic.
  auto verify_state = [&](bool right_aligned, bool rounded_corners,
                          const base::Location& loc = FROM_HERE) {
    SCOPED_TRACE(loc.ToString());
    const bool is_sidebar_leading = (right_aligned == base::i18n::IsRTL());
    const gfx::Insets insets = side_panel->GetInsets();
    const int content_side =
        is_sidebar_leading ? insets.right() : insets.left();
    const int outer_side = is_sidebar_leading ? insets.left() : insets.right();
    if (rounded_corners) {
      EXPECT_EQ(1, content_side)
          << "rounded: 1px gap on the content-facing side, reserved for the "
             "outline (the content view otherwise owns its margin)";
      EXPECT_EQ(kRoundedCornersContentsViewMargin +
                    kRoundedCornersContentsOutlineThickness,
                outer_side)
          << "rounded: outer-side gap matches main content (margin + outline "
             "thickness)";
    } else {
      EXPECT_EQ(1, content_side)
          << "plain: 1px separator on the content-facing side";
      EXPECT_EQ(0, outer_side) << "plain: no outer-side inset";
    }
  };

  // Exercises every (alignment x rounded) combination, transitioning one pref
  // at a time so each step fires OnPreferenceChanged -> UpdateBorder(). Both
  // alignment directions (right->left and left->right) are covered. The matrix
  // ends at right-aligned + plain, so the first step of a second call (a
  // rounded change) is always a real change that recomputes the border.
  auto run_matrix = [&]() {
    prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
    prefs->SetBoolean(kWebViewRoundedCorners, true);
    RunScheduledLayouts();
    ASSERT_TRUE(side_panel->IsRightAligned());
    verify_state(/*right_aligned=*/true, /*rounded_corners=*/true);

    prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
    RunScheduledLayouts();
    ASSERT_FALSE(side_panel->IsRightAligned());
    verify_state(/*right_aligned=*/false, /*rounded_corners=*/true);

    prefs->SetBoolean(kWebViewRoundedCorners, false);
    RunScheduledLayouts();
    verify_state(/*right_aligned=*/false, /*rounded_corners=*/false);

    prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
    RunScheduledLayouts();
    ASSERT_TRUE(side_panel->IsRightAligned());
    verify_state(/*right_aligned=*/true, /*rounded_corners=*/false);
  };

  // LTR.
  run_matrix();

  // RTL: the same stored-space expectations map to the mirror-image visual
  // sides. The first step inside the scope is a rounded-corners change, so the
  // border is recomputed with IsRTL() == true.
  {
    base::i18n::ScopedRTLForTesting scoped_rtl(/*rtl=*/true);
    run_matrix();
  }
}

// Regression test: the top container separator must remain visible when a
// side panel opens. Upstream's CalculateSeparatorInfo() would clear
// `top_container_separator` in this case, but
// BraveBrowserViewTabbedLayoutImpl::CalculateSeparatorInfo() promotes it back
// to true when rounded corners are off, so the separator stays shown.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest,
                       TopContainerSeparatorVisibleWhenPanelOpens) {
  browser()->GetProfile()->GetPrefs()->SetBoolean(kWebViewRoundedCorners,
                                                  false);
  RunScheduledLayouts();

  auto* brave_view =
      BraveBrowserView::From(BrowserView::GetBrowserViewForBrowser(browser()));
  auto* separator = brave_view->top_container_separator_for_testing();
  ASSERT_TRUE(separator);
  EXPECT_TRUE(separator->GetVisible());

  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  panel_ui->Show(SidePanelEntryId::kBookmarks);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return panel_ui->IsSidePanelShowing(); }));

  EXPECT_TRUE(separator->GetVisible());
}

class ScopedSidePanelUIForTesting {
 public:
  ScopedSidePanelUIForTesting(SidebarController* controller, SidePanelUI* ui)
      : controller_(controller) {
    controller_->SetSidePanelUIForTesting(ui);
  }
  ~ScopedSidePanelUIForTesting() {
    controller_->SetSidePanelUIForTesting(nullptr);
  }

 private:
  raw_ptr<SidebarController> controller_;
};

class MockSidePanelUI : public SidePanelUI {
 public:
  MOCK_METHOD(void,
              Show,
              (SidePanelEntryId, std::optional<SidePanelOpenTrigger>, bool),
              (override));
  MOCK_METHOD(void,
              Show,
              (SidePanelEntryKey, std::optional<SidePanelOpenTrigger>, bool),
              (override));
  MOCK_METHOD(void, ShowFrom, (SidePanelEntryKey, gfx::Rect), (override));
  MOCK_METHOD(void, Close, (SidePanelEntryHideReason, bool), (override));
  MOCK_METHOD(void,
              Toggle,
              (SidePanelEntryKey, SidePanelOpenTrigger),
              (override));
  MOCK_METHOD(std::optional<SidePanelEntryId>,
              GetCurrentEntryId,
              (),
              (const, override));
  MOCK_METHOD(int, GetCurrentEntryDefaultContentWidth, (), (const, override));
  MOCK_METHOD(bool, IsSidePanelShowing, (), (const, override));
  MOCK_METHOD(bool,
              IsSidePanelEntryShowing,
              (const SidePanelEntryKey&),
              (const, override));
  MOCK_METHOD(bool,
              IsSidePanelEntryShowing,
              (const SidePanelEntry::Key&, bool),
              (const, override));
  MOCK_METHOD(base::CallbackListSubscription,
              RegisterSidePanelShown,
              (ShownCallback),
              (override));
  MOCK_METHOD(void,
              OnActiveTabChanged,
              (content::WebContents*, content::WebContents*, bool),
              (override));
  MOCK_METHOD(content::WebContents*,
              GetWebContentsForTest,
              (SidePanelEntryId),
              (override));
  MOCK_METHOD(void, DisableAnimationsForTesting, (), (override));
  MOCK_METHOD(void, SetNoDelaysForTesting, (bool), (override));
};

// Verify suppress_animations is false when opening from a closed state and
// true when switching panels while one is already active.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ActivatePanelItemSuppressAnimation) {
  MockSidePanelUI mock_ui;
  ScopedSidePanelUIForTesting scoped_ui(controller(), &mock_ui);

  // No active panel: opening should animate (suppress_animations=false).
  ASSERT_FALSE(model()->active_index().has_value())
      << "Expected no active panel before first ActivatePanelItem call";
  EXPECT_CALL(mock_ui,
              Show(testing::An<SidePanelEntryId>(), testing::Eq(std::nullopt),
                   /*suppress_animations=*/false));
  controller()->ActivatePanelItem(SidebarItem::BuiltInItemType::kBookmarks);
  testing::Mock::VerifyAndClearExpectations(&mock_ui);
  controller()->UpdateActiveItemState(SidebarItem::BuiltInItemType::kBookmarks);

  // Active panel present: switching panels should suppress animations.
  ASSERT_TRUE(model()->active_index().has_value())
      << "Expected active panel after UpdateActiveItemState";
  EXPECT_CALL(mock_ui,
              Show(testing::An<SidePanelEntryId>(), testing::Eq(std::nullopt),
                   /*suppress_animations=*/true));
  controller()->ActivatePanelItem(SidebarItem::BuiltInItemType::kReadingList);
}

// The toolbar SidePanelButton acts as a "temporal pin" for the sidebar
// control view: clicking it toggles a session-only pinned state that forces
// the sidebar visible regardless of show option. The pinned state must reset
// when the show option changes. The button is hidden under kShowAlways
// because the sidebar is already always visible.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ToolbarButtonPinning) {
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto* sidebar_container = GetSidebarContainerView();
  auto* button = GetSidePanelToolbarButton();
  ASSERT_TRUE(button);
  auto button_highlighted = [&]() {
    return views::InkDrop::Get(button)->GetHighlighted();
  };

  // Park the mouse at the top of the toolbar — well outside the sidebar
  // bounds — so that kShowOnMouseOver doesn't keep the sidebar visible due
  // to an incidental hover. Without this the test is sensitive to wherever
  // the OS leaves the cursor between runs.
  auto* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  ui::test::EventGenerator event_generator(
      views::GetRootWindow(browser_view->GetWidget()),
      browser_view->GetNativeWindow());
  event_generator.MoveMouseTo(
      browser_view->toolbar()->GetBoundsInScreen().CenterPoint());
  ASSERT_FALSE(sidebar_container->IsMouseHovered())
      << "Mouse should not be over the sidebar at test start";

  // Pin → assert visible/highlighted; unpin → assert hidden/unhighlighted.
  // Precondition: pinned=false, sidebar hidden. Used once per non-kShowAlways
  // show option. SCOPED_TRACE makes failures point back to the caller.
  auto verify_pin_cycle = [&]() {
    ASSERT_FALSE(controller()->sidebar_pinned())
        << "Precondition: pinned=false before pin";
    ASSERT_FALSE(sidebar_container->IsSidebarVisible())
        << "Precondition: sidebar hidden before pin";

    // Pin: sidebar snaps visible, button highlights (fade-in).
    button->button_controller()->NotifyClick();
    EXPECT_TRUE(controller()->sidebar_pinned()) << "Pin should set pinned=true";
    EXPECT_TRUE(sidebar_container->IsSidebarVisible())
        << "Pin should force sidebar visible regardless of show option";
    ASSERT_TRUE(base::test::RunUntil([&]() { return button_highlighted(); }))
        << "Pin should highlight the toolbar button";

    // Unpin: pinned flips immediately; button highlight + sidebar visibility
    // unwind asynchronously (animations).
    button->button_controller()->NotifyClick();
    EXPECT_FALSE(controller()->sidebar_pinned())
        << "Unpin should set pinned=false";
    ASSERT_TRUE(base::test::RunUntil([&]() { return !button_highlighted(); }))
        << "Unpin should clear the button highlight";
    ASSERT_TRUE(base::test::RunUntil([&]() {
      return !sidebar_container->IsSidebarVisible();
    })) << "Unpin should hide the sidebar (current show option doesn't keep "
           "it visible without pin)";
  };

  // kShowAlways (the fixture default) → button is hidden because the sidebar
  // is always visible.
  ASSERT_EQ(SidebarService::ShowSidebarOption::kShowAlways,
            service->GetSidebarShowOption());
  EXPECT_FALSE(button->GetVisible())
      << "Toolbar button must be hidden under kShowAlways";

  // Switch to kShowNever so the button becomes visible and the sidebar hides.
  service->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);
  EXPECT_TRUE(button->GetVisible())
      << "Toolbar button must be visible under kShowNever";
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !sidebar_container->IsSidebarVisible();
  })) << "Sidebar should hide after switching to kShowNever";

  {
    SCOPED_TRACE("kShowNever");
    verify_pin_cycle();
  }

  // Pin again, then change the show option → pinned must reset to false and
  // the sidebar must follow the new option (kShowOnMouseOver, mouse parked
  // outside sidebar → hidden).
  button->button_controller()->NotifyClick();
  ASSERT_TRUE(controller()->sidebar_pinned())
      << "Pinning again should set pinned=true (precondition for reset test)";
  ASSERT_TRUE(sidebar_container->IsSidebarVisible())
      << "Sidebar should be visible while pinned (precondition)";
  ASSERT_TRUE(base::test::RunUntil([&]() { return button_highlighted(); }))
      << "Button should be highlighted while pinned (precondition)";

  service->SetSidebarShowOption(
      SidebarService::ShowSidebarOption::kShowOnMouseOver);
  EXPECT_FALSE(controller()->sidebar_pinned())
      << "Changing show option must reset pinned state to false";
  EXPECT_TRUE(button->GetVisible())
      << "Toolbar button must be visible under kShowOnMouseOver";
  ASSERT_TRUE(base::test::RunUntil([&]() { return !button_highlighted(); }))
      << "Button highlight should fade off after pinned reset";
  ASSERT_TRUE(base::test::RunUntil([&]() {
    return !sidebar_container->IsSidebarVisible();
  })) << "Sidebar should hide once pinned is reset under kShowOnMouseOver "
         "(mouse is not over sidebar in tests)";

  {
    SCOPED_TRACE("kShowOnMouseOver");
    verify_pin_cycle();
  }

  // Switching back to kShowAlways re-hides the toolbar button.
  service->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowAlways);
  EXPECT_FALSE(button->GetVisible())
      << "Toolbar button must be hidden again under kShowAlways";
  EXPECT_FALSE(controller()->sidebar_pinned())
      << "Pinned state must remain false after returning to kShowAlways";
}

// Verifies that pinning the sidebar highlights the toolbar button and that the
// highlight survives theme changes (regression test for when OnThemeChanged()
// was clearing the ink-drop highlight state).
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest,
                       ButtonHighlightPersistedAfterThemeChange) {
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto* theme_service =
      ThemeServiceFactory::GetForProfile(browser()->GetProfile());
  auto* button = GetSidePanelToolbarButton();
  ASSERT_TRUE(button);

  // Set initial theme.
  theme_service->SetBrowserColorScheme(
      ThemeService::BrowserColorScheme::kLight);

  auto button_highlighted = [&]() {
    return views::InkDrop::Get(button)->GetHighlighted();
  };

  // Switch to kShowNever so the toolbar button is visible.
  service->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);
  ASSERT_TRUE(button->GetVisible());

  // Pin the sidebar — button should become highlighted.
  button->button_controller()->NotifyClick();
  ASSERT_TRUE(controller()->sidebar_pinned());
  ASSERT_TRUE(base::test::RunUntil([&]() { return button_highlighted(); }))
      << "Button should be highlighted after pinning";

  // Switch to dark theme — this previously cleared the ink-drop highlight.
  theme_service->SetBrowserColorScheme(ThemeService::BrowserColorScheme::kDark);
  EXPECT_TRUE(button_highlighted())
      << "Button highlight should be preserved after switching to dark theme";

  // Switch back to light theme — highlight must survive this change too.
  theme_service->SetBrowserColorScheme(
      ThemeService::BrowserColorScheme::kLight);
  EXPECT_TRUE(button_highlighted())
      << "Button highlight should be preserved after switching to light theme";
}

// Exercises the side panel's own rounded outline (SidePanel::UpdateBorder(),
// via the panel's border -- there's no separate overlay view anymore):
//   * with rounded corners off, the border is a plain 1px separator, no extra
//     top/inner gap is reserved, and
//   * with rounded corners on, the border reserves the wider outer/bottom
//     margin plus a slim 1px gap on the top/inner edges (where the
//     header/content would otherwise be flush with the panel edge), so the
//     outline has room to show on all four sides.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PanelRoundedOutline) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* side_panel = browser_view()->side_panel();
  side_panel->DisableAnimationsForTesting();
  auto* prefs = browser()->GetProfile()->GetPrefs();

  // Growser-151: the insets below name a content-facing (left) and an outer
  // (right) edge, which is the right-hand layout. Growser starts the sidebar
  // on the left, where those two swap, so the layout under test is set here.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);

  // Open the panel with rounded corners off: a plain separator, no top gap.
  prefs->SetBoolean(kWebViewRoundedCorners, false);
  panel_ui->Toggle();
  ASSERT_TRUE(base::test::RunUntil([&]() { return side_panel->GetVisible(); }));
  RunScheduledLayouts();

  // If panel has header, top insets includeis header height.
  int header_height = 0;
  if (auto* header = side_panel->GetHeaderView<BraveSidePanelHeader>()) {
    header_height = header->height();
  }
  EXPECT_EQ(0, side_panel->GetInsets().top() - header_height)

      << "no rounded corners: no top gap reserved for an outline";

  // Turn rounded corners on: the border reserves the outer/bottom margin plus
  // a 1px gap on the top and content-facing (inner) edges.
  prefs->SetBoolean(kWebViewRoundedCorners, true);
  RunScheduledLayouts();
  {
    const gfx::Insets insets = side_panel->GetInsets();
    constexpr int kOutlineThickness = 1;
    EXPECT_EQ(kOutlineThickness, insets.top() - header_height)
        << "rounded corners: 1px top gap reserved for the outline";
    EXPECT_EQ(kOutlineThickness, insets.left())
        << "rounded corners: 1px content-facing gap reserved for the outline";
    EXPECT_EQ(kRoundedCornersContentsViewMargin +
                  kRoundedCornersContentsOutlineThickness,
              insets.right())
        << "rounded corners: outer-side gap matches main content (margin + "
           "outline thickness)";
  }

  // Disabling rounded corners drops the extra top/inner gap again.
  prefs->SetBoolean(kWebViewRoundedCorners, false);
  RunScheduledLayouts();
  EXPECT_EQ(0, side_panel->GetInsets().top() - header_height)
      << "no rounded corners: no top gap reserved for an outline";
}

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, SidebarItemHighlightState) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  auto* side_panel = browser_view()->side_panel();
  auto* service = SidebarServiceFactory::GetForProfile(browser()->GetProfile());
  auto items_contents_view = GetSidebarItemsContentsView(controller());
  SidebarContainerView* sidebar = GetSidebarContainerView();

  side_panel->DisableAnimationsForTesting();

  // Start with sidebar hidden so we can drive visibility entirely through
  // ToggleSidebarPinning below.
  service->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return !sidebar->IsSidebarVisible(); }));

  // Open a panel to establish an active sidebar item whose ink drop we can
  // inspect. The sidebar must still be hidden afterwards (kShowNever).
  panel_ui->Toggle();
  ASSERT_TRUE(
      base::test::RunUntil([&]() { return panel_ui->IsSidePanelShowing(); }));
  RunScheduledLayouts();
  EXPECT_FALSE(sidebar->IsSidebarVisible());
  const auto active_index = model()->active_index();
  EXPECT_TRUE(active_index);

  // Show the sidebar and verify the active item's ink drop is highlighted.
  // This was broken: ShowSidebar() didn't re-sync item state, and
  // SetActiveState() short-circuited when active_ hadn't changed, so the ink
  // drop remained HIDDEN even though the item was active.
  controller()->ToggleSidebarPinning();
  EXPECT_TRUE(sidebar->IsSidebarVisible());
  auto* active_item_view = views::AsViewClass<SidebarItemView>(
      items_contents_view->children()[*active_index].get());
  ASSERT_TRUE(active_item_view);
  auto* ink_drop = views::InkDrop::Get(active_item_view)->GetInkDrop();
  EXPECT_EQ(views::InkDropState::ACTIVATED, ink_drop->GetTargetInkDropState());

  // Hide and re-show to confirm the fix holds across multiple toggle cycles.
  controller()->ToggleSidebarPinning();
  EXPECT_FALSE(sidebar->IsSidebarVisible());
  EXPECT_EQ(views::InkDropState::HIDDEN, ink_drop->GetTargetInkDropState());

  controller()->ToggleSidebarPinning();
  EXPECT_TRUE(sidebar->IsSidebarVisible());
  EXPECT_EQ(views::InkDropState::ACTIVATED, ink_drop->GetTargetInkDropState());
}

// Verify that SidebarControlView::UpdateBorder() applies a negative inset on
// the content-facing side equal to -kRoundedCornersContentsViewMargin when
// rounded corners is on, so the control view overlaps the margin already owned
// by the contents/panel and avoids a double gap. When rounded corners is off
// the margin is 0, so the border insets are all zero.
//
// Also verifies the relationship between kSidebarButtonSize, kMargin, and the
// control view's preferred width: the FlexLayout adds border insets to the max
// child width, so a negative border shrinks the preferred width by the overlap
// margin.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, ControlViewBorderOverlapGap) {
  auto* prefs = browser()->GetProfile()->GetPrefs();
  auto* control_view = GetSidebarControlView();
  const int button_width =
      SidebarButtonView::kSidebarButtonSize + SidebarButtonView::kMargin * 2;

  // Growser-151: growser's default is the left; the insets under test belong
  // to the right-hand layout, so it is set here.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, true);
  RunScheduledLayouts();
  ASSERT_FALSE(IsSidebarUIOnLeft());

  // Rounded corners off: no overlap needed, all insets should be zero and the
  // control view's preferred width equals exactly the button width.
  prefs->SetBoolean(kWebViewRoundedCorners, false);
  RunScheduledLayouts();
  {
    const gfx::Insets insets = control_view->GetBorder()->GetInsets();
    EXPECT_EQ(0, insets.left()) << "rounded corners off: no left inset";
    EXPECT_EQ(0, insets.right()) << "rounded corners off: no right inset";
    EXPECT_EQ(button_width, control_view->GetPreferredSize().width())
        << "no border: control view preferred width equals button width";
  }

  // Rounded corners on, sidebar on right: content-facing side is left, so the
  // left inset should be -kRoundedCornersContentsViewMargin. The negative
  // border reduces the preferred width by the overlap margin.
  prefs->SetBoolean(kWebViewRoundedCorners, true);
  RunScheduledLayouts();
  {
    const gfx::Insets insets = control_view->GetBorder()->GetInsets();
    EXPECT_EQ(-kRoundedCornersContentsViewMargin, insets.left())
        << "sidebar on right: left inset overlaps into content margin";
    EXPECT_EQ(0, insets.right());
    EXPECT_EQ(button_width - kRoundedCornersContentsViewMargin,
              control_view->GetPreferredSize().width())
        << "rounded corners on: preferred width narrows by the overlap margin";
  }

  // Flip to left-aligned: content-facing side becomes right.
  prefs->SetBoolean(prefs::kSidePanelHorizontalAlignment, false);
  RunScheduledLayouts();
  ASSERT_TRUE(IsSidebarUIOnLeft());
  {
    const gfx::Insets insets = control_view->GetBorder()->GetInsets();
    EXPECT_EQ(0, insets.left());
    EXPECT_EQ(-kRoundedCornersContentsViewMargin, insets.right())
        << "sidebar on left: right inset overlaps into content margin";
    EXPECT_EQ(button_width - kRoundedCornersContentsViewMargin,
              control_view->GetPreferredSize().width())
        << "rounded corners on: preferred width narrows by the overlap margin";
  }
}

class SidebarBrowserTestWithFocusMode : public SidebarBrowserTest {
 public:
  SidebarBrowserTestWithFocusMode() {
    feature_list_.InitAndEnableFeature(::features::kBraveFocusMode);
  }
  ~SidebarBrowserTestWithFocusMode() override = default;

  FocusModeController* focus_mode_controller() {
    return browser()->GetFeatures().focus_mode_controller();
  }

  void SetSidebarShowOption(SidebarService::ShowSidebarOption option) {
    SidebarServiceFactory::GetForProfile(browser()->GetProfile())
        ->SetSidebarShowOption(option);
  }

  // Moves the cursor into the sidebar's hot corner and dispatches a mouse move,
  // mirroring the mechanism used by ShowSidebarOnMouseOverTest.
  void MoveMouseToSidebarHotCorner() {
    browser_view()->DeprecatedLayoutImmediately();
    browser()->GetProfile()->GetPrefs()->SetBoolean(
        prefs::kSidePanelHorizontalAlignment, false);

    gfx::Point hot_corner =
        browser_view()->GetBoundingBoxInScreenForMouseOverHandling().origin();
    hot_corner.Offset(2, 2);
    display::Screen::Get()->SetCursorScreenPointForTesting(hot_corner);
    HandleBrowserWindowMouseEvent();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  display::test::TestScreen screen_{/*create_display=*/true,
                                    /*register_screen=*/true};
};

// While Focus Mode is active, an always-shown sidebar switches to hover-driven
// auto-hide, then returns to always-shown when Focus Mode is disabled.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithFocusMode,
                       AlwaysShownSidebarAutoHidesInFocusMode) {
  SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowAlways);

  auto* sidebar_container = GetSidebarContainerView();
  ASSERT_TRUE(focus_mode_controller());
  ASSERT_TRUE(sidebar_container->IsSidebarVisible());

  // Enabling Focus Mode auto-hides the always-shown sidebar.
  focus_mode_controller()->SetEnabled(true);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  // The sidebar is now hover-driven: moving into the hot corner reveals it.
  MoveMouseToSidebarHotCorner();
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());

  // Disabling Focus Mode restores the always-shown behavior.
  focus_mode_controller()->SetEnabled(false);
  EXPECT_TRUE(sidebar_container->IsSidebarVisible());
}

// Focus Mode must not reveal a sidebar the user has chosen to never show.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTestWithFocusMode,
                       NeverShownSidebarStaysHiddenInFocusMode) {
  SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);

  auto* sidebar_container = GetSidebarContainerView();
  ASSERT_FALSE(sidebar_container->IsSidebarVisible());

  focus_mode_controller()->SetEnabled(true);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());

  focus_mode_controller()->SetEnabled(false);
  EXPECT_FALSE(sidebar_container->IsSidebarVisible());
}

// Growser-140
// Pinned tabs in the sidebar (growser#140). The rule these tests hold the
// feature to is that a pinned tab is drawn exactly once: what the sidebar
// hosts leaves the tab strip, and what the sidebar cannot fit stays there.
class SidebarPinnedTabsBrowserTest : public SidebarBrowserTest {
 public:
  SidebarPinnedTabsBrowserTest() = default;
  ~SidebarPinnedTabsBrowserTest() override = default;

 protected:
  static constexpr int kPinnedTabCount = 3;

  void SetUpOnMainThread() override {
    SidebarBrowserTest::SetUpOnMainThread();

    SidebarServiceFactory::GetForProfile(browser()->GetProfile())
        ->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowAlways);

    for (int i = 0; i < kPinnedTabCount; i++) {
      chrome::AddTabAt(browser(), GURL(url::kAboutBlankURL), -1, false);
      tab_model()->SetTabPinned(i, true);
    }
    ASSERT_EQ(kPinnedTabCount, tab_model()->IndexOfFirstNonPinnedTab());
  }

  void SetShowPinnedTabs(bool enabled) {
    browser()->GetProfile()->GetPrefs()->SetBoolean(kSidebarShowPinnedTabs,
                                                    enabled);
    RunLayout();
  }

  void RunLayout() {
    views::test::RunScheduledLayout(browser_view());
    browser_view()->DeprecatedLayoutImmediately();
  }

  int HostedBySidebar() const {
    return GetSidebarPinnedTabsView()->hosted_count();
  }

  // Pinned tabs the strip is actually drawing.
  int DrawnByTabStrip() {
    auto* tab_strip = browser_view()->horizontal_tab_strip_for_testing();
    int drawn = 0;
    for (int i = 0; i < tab_model()->IndexOfFirstNonPinnedTab(); i++) {
      if (tab_strip->tab_at(i)->GetVisible()) {
        drawn++;
      }
    }
    return drawn;
  }

  // The invariant the whole feature rests on: every pinned tab is on exactly
  // one of the two surfaces.
  void ExpectNoTabLostOrDoubled() {
    EXPECT_EQ(kPinnedTabCount, HostedBySidebar() + DrawnByTabStrip());
  }

  // Growser-165
  views::View* EntryAt(int index) {
    return GetSidebarPinnedTabsView()->children()[index];
  }

  // The gesture is driven at the view layer rather than with
  // ui::test::EventGenerator, and that is deliberate. Measured in this suite:
  // a generated left button never reaches this widget at all - after
  // MoveMouseTo the entry does not even go to STATE_HOVERED - while a right
  // click through the same generator does arrive and opens a menu
  // (RightClickOpensAMenu). Rather than test the harness, these drive the
  // handlers directly and assert separately, with EventHandlerAt(), that a
  // real click at that point would be delivered to the entry.
  ui::MouseEvent MouseEventAt(ui::EventType type, const gfx::Point& p) {
    return ui::MouseEvent(type, p, p, ui::EventTimeForNow(),
                          ui::EF_LEFT_MOUSE_BUTTON, ui::EF_LEFT_MOUSE_BUTTON);
  }

  // Presses on the entry at |from| and drags through |waypoints| (given in the
  // block's coordinates), releasing at the last one. |recognised| reports
  // whether the block ever saw the gesture become a drag, so a caller can tell
  // "never recognised" from "recognised and dropped nowhere" - the model alone
  // says the same thing for both.
  void DragEntryThrough(int from,
                        const std::vector<gfx::Point>& waypoints,
                        bool* recognised = nullptr) {
    views::View* entry = EntryAt(from);
    entry->OnMousePressed(MouseEventAt(ui::EventType::kMousePressed,
                                       entry->GetLocalBounds().CenterPoint()));

    gfx::Point last;
    for (const gfx::Point& waypoint : waypoints) {
      last = waypoint;
      views::View::ConvertPointToTarget(GetSidebarPinnedTabsView(), entry,
                                        &last);
      entry->OnMouseDragged(MouseEventAt(ui::EventType::kMouseDragged, last));
    }
    if (recognised) {
      *recognised = GetSidebarPinnedTabsView()->IsDraggingForTesting();
    }
    entry->OnMouseReleased(MouseEventAt(ui::EventType::kMouseReleased, last));
    RunLayout();
  }

  void DragEntryTo(int from,
                   const gfx::Point& to,
                   bool* recognised = nullptr) {
    DragEntryThrough(from, {to}, recognised);
  }

  // The gap above the entry at |index|, in the block's coordinates - where a
  // drop puts the dragged tab.
  gfx::Point GapAbove(int index) {
    const gfx::Rect bounds = EntryAt(index)->bounds();
    return gfx::Point(bounds.CenterPoint().x(), bounds.y() + 1);
  }

  // Which view a click at |point_in_screen| would actually be delivered to.
  // A test that presses somewhere and asserts on the result cannot tell "the
  // handler ignored it" from "something else was on top", and those need
  // different fixes.
  views::View* EventHandlerAt(const gfx::Point& point_in_screen) {
    views::View* root = browser_view()->GetWidget()->GetRootView();
    gfx::Point local = point_in_screen;
    views::View::ConvertPointFromScreen(root, &local);
    return root->GetEventHandlerForPoint(local);
  }

  std::vector<content::WebContents*> PinnedContents() {
    std::vector<content::WebContents*> contents;
    for (int i = 0; i < tab_model()->IndexOfFirstNonPinnedTab(); i++) {
      contents.push_back(tab_model()->GetWebContentsAt(i));
    }
    return contents;
  }
};

// Off by default, and turning it on moves the pinned tabs out of the strip.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, MovesPinnedTabsAndBack) {
  EXPECT_FALSE(
      browser()->GetProfile()->GetPrefs()->GetBoolean(kSidebarShowPinnedTabs));
  EXPECT_EQ(0, HostedBySidebar());
  EXPECT_EQ(kPinnedTabCount, DrawnByTabStrip());

  SetShowPinnedTabs(true);
  EXPECT_EQ(kPinnedTabCount, HostedBySidebar());
  EXPECT_EQ(0, DrawnByTabStrip());
  ExpectNoTabLostOrDoubled();

  // Turning it back off hands every one of them back.
  SetShowPinnedTabs(false);
  EXPECT_EQ(0, HostedBySidebar());
  EXPECT_EQ(kPinnedTabCount, DrawnByTabStrip());
}

// Unpinning a tab the sidebar hosts returns it to the strip as a normal tab.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, UnpinReturnsTheTab) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  tab_model()->SetTabPinned(0, false);
  RunLayout();

  EXPECT_EQ(kPinnedTabCount - 1, tab_model()->IndexOfFirstNonPinnedTab());
  EXPECT_EQ(kPinnedTabCount - 1, HostedBySidebar());
  EXPECT_EQ(0, DrawnByTabStrip());
}

// The split follows the window height, in both directions, with no user action.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, FollowsTheWindowHeight) {
  SetShowPinnedTabs(true);
  const gfx::Rect original_bounds =
      browser_view()->GetWidget()->GetWindowBoundsInScreen();
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  // Shrink until the sidebar cannot hold them all. Growser-149 moved the block
  // to the top of the sidebar and took away the divider above it, so it fits
  // more than it used to - a single hard-coded height stopped being short
  // enough, and would quietly stop testing anything again after the next
  // layout change. The invariant is checked at every step, which is the part
  // that matters: whatever the split, no tab is ever on neither surface.
  gfx::Rect short_bounds = original_bounds;
  for (int height = original_bounds.height(); height >= 200; height -= 50) {
    short_bounds.set_height(height);
    browser_view()->GetWidget()->SetBounds(short_bounds);
    RunLayout();
    ExpectNoTabLostOrDoubled();
  }

  EXPECT_LT(HostedBySidebar(), kPinnedTabCount)
      << "a window this short must hand pinned tabs back to the strip";

  // And growing it takes them back.
  browser_view()->GetWidget()->SetBounds(original_bounds);
  RunLayout();

  EXPECT_EQ(kPinnedTabCount, HostedBySidebar());
  ExpectNoTabLostOrDoubled();
}

// Vertical tabs already show pinned tabs in a column of their own, so the
// sidebar hosts nothing while they are on - the setting is inert, not a way to
// make pinned tabs disappear from both places.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, InertWithVerticalTabs) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  browser()->GetProfile()->GetPrefs()->SetBoolean(
      brave_tabs::kVerticalTabsEnabled, true);
  RunLayout();

  EXPECT_EQ(0, HostedBySidebar());
}

// Control for the two tests below: pinned tabs exist and the sidebar is hidden,
// but the feature was never turned on. Tells a fault in the hosting path apart
// from one in the fixture itself.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest,
                       ControlHiddenSidebarWithFeatureOff) {
  SidebarServiceFactory::GetForProfile(browser()->GetProfile())
      ->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);
  RunLayout();

  EXPECT_EQ(0, HostedBySidebar());
  EXPECT_EQ(kPinnedTabCount, DrawnByTabStrip());
}

// A sidebar that is not permanently visible hosts nothing: hiding it must not
// take pinned tabs with it.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest,
                       HiddenSidebarHostsNothing) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  SidebarServiceFactory::GetForProfile(browser()->GetProfile())
      ->SetSidebarShowOption(SidebarService::ShowSidebarOption::kShowNever);
  RunLayout();

  EXPECT_EQ(0, HostedBySidebar());
  EXPECT_EQ(kPinnedTabCount, DrawnByTabStrip());
}


// Growser-147
// A sidebar entry's right click opens the menu of the tab it mirrors
// (growser#147). What the menu contains is not asserted here on purpose: the
// view hands the click to TabStrip::ShowContextMenuForTab, so it is the tab's
// own menu by construction - model, Brave items and the delegate that knows
// what Brave's commands mean. What can go wrong here, and so is tested, is
// which tab it asks for.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, EntriesPointAtTheirTabs) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  auto* view = GetSidebarPinnedTabsView();
  auto* tab_strip = browser_view()->horizontal_tab_strip_for_testing();

  for (int i = 0; i < kPinnedTabCount; i++) {
    Tab* tab = view->GetTabForEntryForTesting(static_cast<size_t>(i));
    ASSERT_TRUE(tab) << "entry " << i;
    EXPECT_EQ(tab_strip->tab_at(i), tab);
    EXPECT_TRUE(tab->data().pinned);
  }

  // Past the end, and past the pinned tabs, there is no tab to ask about.
  EXPECT_FALSE(view->GetTabForEntryForTesting(kPinnedTabCount));
}

// A right click on an entry really opens a menu. The test above proves the
// right tab is asked about; this one proves the click gets that far at all -
// the button could as easily swallow it, which is not visible from the code.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, RightClickOpensAMenu) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  views::View* entry = GetSidebarPinnedTabsView()->children()[1];
  ASSERT_TRUE(entry->GetVisible());

  ui::test::EventGenerator generator(
      views::GetRootWindow(browser_view()->GetWidget()));
  generator.MoveMouseTo(entry->GetBoundsInScreen().CenterPoint());
  generator.ClickRightButton();

  views::MenuController* menu = views::MenuController::GetActiveInstance();
  EXPECT_TRUE(menu) << "no menu came up for a right click on a pinned entry";
  if (menu) {
    menu->Cancel(views::MenuController::ExitType::kAll);
  }
}

// Unpinning through the menu removes the entry the menu belongs to. Runs the
// command the way a menu click does - through the tab strip's controller - so
// the sidebar is exercised as a bystander, which is what it is.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, UnpinFromTheEntrysMenu) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  // The strip's controller forwards a menu click straight to the model, so this
  // is the same call the "Unpin tab" item makes.
  tab_model()->ExecuteContextMenuCommand(0, TabStripModel::CommandTogglePinned);
  RunLayout();

  EXPECT_EQ(kPinnedTabCount - 1, tab_model()->IndexOfFirstNonPinnedTab());
  EXPECT_EQ(kPinnedTabCount - 1, HostedBySidebar());
  EXPECT_EQ(0, DrawnByTabStrip());
}


// Growser-165
// Dragging an entry inside the sidebar reorders the pinned tabs. What is
// asserted is the MODEL, not the views: entry i mirrors model index i, so the
// entries stay exactly where they are and redraw with the tab that is now at
// their index. Watching a view move would be watching the wrong thing.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, DragReordersPinnedTabs) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  const std::vector<content::WebContents*> before = PinnedContents();
  ASSERT_EQ(3u, before.size());

  views::View* handler =
      EventHandlerAt(EntryAt(2)->GetBoundsInScreen().CenterPoint());
  ASSERT_EQ(EntryAt(2), handler)
      << "the press would go to "
      << (handler ? handler->GetClassName() : "nothing");

  bool recognised = false;
  DragEntryTo(2, GapAbove(0), &recognised);
  EXPECT_TRUE(recognised) << "the block never saw the gesture become a drag";

  EXPECT_EQ(before[2], tab_model()->GetWebContentsAt(0));
  EXPECT_EQ(before[0], tab_model()->GetWebContentsAt(1));
  EXPECT_EQ(before[1], tab_model()->GetWebContentsAt(2));
  EXPECT_EQ(kPinnedTabCount, tab_model()->IndexOfFirstNonPinnedTab())
      << "the drag must not unpin anything";
  ExpectNoTabLostOrDoubled();
}

// Dropping an entry back where it started is not a move. Worth its own test
// because the indicator index and the target index are not the same number,
// and getting that wrong shifts every drop by one. The gesture goes away and
// comes back so that it is unmistakably a drag by the time it is released.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest,
                       DragToItsOwnPlaceChangesNothing) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  const std::vector<content::WebContents*> before = PinnedContents();

  bool recognised = false;
  DragEntryThrough(1, {GapAbove(0), GapAbove(1)}, &recognised);
  ASSERT_TRUE(recognised);

  EXPECT_EQ(before, PinnedContents());
  ExpectNoTabLostOrDoubled();
}

// A press and release that never became a drag is still a click, and a click
// still activates the tab. The drag code sits on the same three events, so
// this is the thing it can break without breaking anything else.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest, AClickIsStillAClick) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  const std::vector<content::WebContents*> before = PinnedContents();
  ASSERT_NE(2, tab_model()->active_index());

  // That a real click would land here, rather than on something covering the
  // entry, is the half this test cannot drive itself.
  const gfx::Point center_in_screen =
      EntryAt(2)->GetBoundsInScreen().CenterPoint();
  views::View* handler = EventHandlerAt(center_in_screen);
  ASSERT_EQ(EntryAt(2), handler)
      << "a click on the entry would go to "
      << (handler ? handler->GetClassName() : "nothing");

  views::View* entry = EntryAt(2);
  const gfx::Point center = entry->GetLocalBounds().CenterPoint();
  entry->OnMousePressed(MouseEventAt(ui::EventType::kMousePressed, center));
  EXPECT_EQ(views::Button::STATE_PRESSED,
            GetSidebarPinnedTabsView()->GetEntryForTesting(2)->GetState())
      << "the press did not put the entry's button in its pressed state";
  entry->OnMouseReleased(MouseEventAt(ui::EventType::kMouseReleased, center));
  RunLayout();

  EXPECT_EQ(2, tab_model()->active_index());
  EXPECT_EQ(before, PinnedContents()) << "a click must not reorder anything";
}

// Brave's own sidebar buttons take no part in this: an entry dragged out of
// the pinned block and dropped on them does nothing at all. The block reaches
// exactly as far as the entries it shows, so there is no gap out there to fall
// into - and because the gesture did become a drag, the release must not be
// treated as a click on the entry either.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest,
                       DraggingOntoTheLegacyButtonsDoesNothing) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  const std::vector<content::WebContents*> before = PinnedContents();

  // Well below the last entry, where Brave's own buttons live.
  const gfx::Rect last = EntryAt(kPinnedTabCount - 1)->bounds();
  bool recognised = false;
  DragEntryTo(0, gfx::Point(last.CenterPoint().x(), last.bottom() + 60),
              &recognised);
  ASSERT_TRUE(recognised);

  EXPECT_EQ(before, PinnedContents());
  ExpectNoTabLostOrDoubled();
}

// A gesture that became a drag is not a click, even when it is released back
// over the entry it started on. Without that, reordering a pinned tab and
// changing your mind would also switch to it - and the suppression lives in
// IsTriggerableEvent(), which nothing else here would exercise, because every
// other drag ends somewhere the button would not have fired anyway.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsBrowserTest,
                       ADragThatReturnsHomeIsNotAClick) {
  SetShowPinnedTabs(true);
  ASSERT_EQ(kPinnedTabCount, HostedBySidebar());

  const std::vector<content::WebContents*> before = PinnedContents();
  const int active_before = tab_model()->active_index();
  ASSERT_NE(2, active_before);

  const gfx::Rect entry = EntryAt(2)->bounds();
  bool recognised = false;
  DragEntryThrough(2, {GapAbove(0), entry.CenterPoint()}, &recognised);
  ASSERT_TRUE(recognised);

  EXPECT_EQ(active_before, tab_model()->active_index())
      << "a drag released over its own entry must not count as a click";
  EXPECT_EQ(before, PinnedContents());
  ExpectNoTabLostOrDoubled();
}


// Growser-148
// Containers are behind a feature, and the accent is gated on it - so this
// lives in its own fixture rather than turning the feature on for every test
// above.
class SidebarPinnedTabsContainerBrowserTest
    : public SidebarPinnedTabsBrowserTest {
 public:
  SidebarPinnedTabsContainerBrowserTest() {
    feature_list_.InitAndEnableFeature(containers::features::kContainers);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// A pinned tab in a container wears the container's accent in the sidebar, and
// wears the tab strip's answer rather than one worked out again here: the test
// compares against what BraveTabStrip gives for the same tab, so the two
// surfaces cannot drift apart without this failing.
IN_PROC_BROWSER_TEST_F(SidebarPinnedTabsContainerBrowserTest,
                       EntryWearsContainerAccent) {
  containers::SetContainersEnabled(true, browser()->GetProfile()->GetPrefs());

  auto container = containers::mojom::Container::New();
  container->id = "growser-148-test";
  container->name = "Test";
  container->icon = containers::mojom::Icon::kWork;
  container->background_color = SK_ColorBLUE;
  // A real URL, not about:blank: the container is carried by the storage
  // partition the tab is created with, and about:blank does not get one.
  ASSERT_TRUE(embedded_test_server()->Start());
  brave::OpenUrlInContainer(browser(),
                            embedded_test_server()->GetURL("/title1.html"),
                            container);

  content::WebContents* contents = tab_model()->GetActiveWebContents();
  ASSERT_TRUE(contents);
  tab_model()->SetTabPinned(tab_model()->GetIndexOfWebContents(contents), true);
  const int index = tab_model()->GetIndexOfWebContents(contents);
  ASSERT_LT(index, tab_model()->IndexOfFirstNonPinnedTab());

  // Capture what the strip draws for this tab BEFORE the sidebar takes it
  // over: a hosted tab is hidden, and a hidden view paints nothing, so a
  // capture taken afterwards would be blank and prove nothing.
  {
    auto* strip_before = views::AsViewClass<BraveTabStrip>(
        browser_view()->horizontal_tab_strip_for_testing());
    Tab* tab_before = strip_before->tab_at(index);
    if (const char* dir = getenv("GROWSER_TEST_PNG_DIR")) {
      SkBitmap tab_bitmap;
      const gfx::Size tab_size(tab_before->bounds().right(), tab_before->bounds().bottom());
      {
        ui::CanvasPainter tab_painter(&tab_bitmap, tab_size, 1.f,
                                      SK_ColorTRANSPARENT, false);
        tab_before->Paint(views::PaintInfo::CreateRootPaintInfo(tab_painter.context(),
                                                         tab_size));
      }
      std::optional<std::vector<uint8_t>> tab_png =
          gfx::PNGCodec::EncodeBGRASkBitmap(tab_bitmap, false);
      if (tab_png) {
        base::WriteFile(base::FilePath::FromUTF8Unsafe(dir).AppendASCII(
                            "tabstrip-pinned-tab.png"),
                        *tab_png);
      }
    }

  }

  SetShowPinnedTabs(true);
  ASSERT_GT(HostedBySidebar(), index);

  auto* tab_strip = views::AsViewClass<BraveTabStrip>(
      browser_view()->horizontal_tab_strip_for_testing());
  Tab* tab = tab_strip->tab_at(index);
  ASSERT_TRUE(tab_strip->ShouldPaintTabAccent(tab))
      << "the tab strip does not think this tab is in a container";

  auto* entry = GetSidebarPinnedTabsView()->GetEntryForTesting(index);
  ASSERT_TRUE(entry);
  ASSERT_TRUE(entry->accent_colors().has_value());

  const auto expected = tab_strip->GetTabAccentColors(tab);
  ASSERT_TRUE(expected.has_value());
  EXPECT_EQ(expected->border_color, entry->accent_colors()->border_color);
  EXPECT_EQ(expected->background_color,
            entry->accent_colors()->background_color);
  EXPECT_EQ(expected->icon_border_color,
            entry->accent_colors()->icon_border_color);
  EXPECT_TRUE(entry->has_accent_icon());

  // Painted, not just stored - but the accent lives on its own layer (it has
  // to, to sit above the ink drop), and a layer is absent from any capture of
  // the button's canvas. So the overlay is painted directly and the accent
  // colours are looked for in that.
  {
    views::View* overlay = entry->GetAccentOverlayForTesting();
    ASSERT_TRUE(overlay);
    EXPECT_TRUE(entry->IsAccentOnItsOwnLayer());
    EXPECT_EQ(entry->GetContentsBounds().size(), overlay->size())
        << "the accent overlay does not cover the button";

    SkBitmap bitmap;
    const gfx::Size size(overlay->bounds().right(), overlay->bounds().bottom());
    {
      // CanvasPainter rasterises into the bitmap in its destructor: reading it
      // while the painter is alive reports "nothing painted" for a view that
      // painted fine.
      ui::CanvasPainter painter(&bitmap, size, 1.f, SK_ColorTRANSPARENT, false);
      overlay->Paint(
          views::PaintInfo::CreateRootPaintInfo(painter.context(), size));
    }

    int accent_pixels = 0;
    for (int x = 0; x < size.width(); x++) {
      for (int y = 0; y < size.height(); y++) {
        const SkColor color = bitmap.getColor(x, y);
        if (color == expected->background_color ||
            color == expected->border_color) {
          accent_pixels++;
        }
      }
    }

    if (const char* dir = getenv("GROWSER_TEST_PNG_DIR")) {
      std::optional<std::vector<uint8_t>> png =
          gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false);
      LOG(ERROR) << "overlay " << size.ToString() << " accent=" << accent_pixels;
      if (png) {
        base::WriteFile(base::FilePath::FromUTF8Unsafe(dir).AppendASCII(
                            "sidebar-entry-accent.png"),
                        *png);
      }
    }

    EXPECT_GT(accent_pixels, 0) << "the accent never reached the pixels";

    // Growser-150: the light for the active tab is painted on the entry's own
    // canvas (it has to be, to sit under the favicon), so unlike the accent it
    // does show up in a canvas capture - of the whole block, so that "only the
    // active entry has it" is visible in one picture.
    if (const char* dir = getenv("GROWSER_TEST_PNG_DIR")) {
      views::View* block = GetSidebarPinnedTabsView();
      SkBitmap block_bitmap;
      const gfx::Size block_size(block->bounds().right(),
                                 block->bounds().bottom());
      {
        ui::CanvasPainter block_painter(&block_bitmap, block_size, 1.f,
                                        SK_ColorTRANSPARENT, false);
        block->Paint(views::PaintInfo::CreateRootPaintInfo(
            block_painter.context(), block_size));
      }
      std::optional<std::vector<uint8_t>> block_png =
          gfx::PNGCodec::EncodeBGRASkBitmap(block_bitmap, false);
      if (block_png) {
        base::WriteFile(base::FilePath::FromUTF8Unsafe(dir).AppendASCII(
                            "sidebar-block-live.png"),
                        *block_png);
      }
    }
  }


  // A tab in no container is drawn plain - the entries the fixture pinned.
  auto* plain = GetSidebarPinnedTabsView()->GetEntryForTesting(0);
  ASSERT_TRUE(plain);
  EXPECT_FALSE(plain->accent_colors().has_value());
  EXPECT_FALSE(plain->has_accent_icon());
}


// Growser-149
// The sidebar's shape: pinned tabs at the very top, and what is left of the
// legacy sidebar at the bottom with the settings gear behind a separator. No
// way to add or rearrange those buttons any more - the add button is not built
// at all, and neither the items nor the sidebar itself carries a context menu.
IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, PinnedTabsOnTopLegacyAtTheBottom) {
  auto* control = GetSidebarControlView();
  ASSERT_TRUE(control);

  const auto& children = control->children();
  ASSERT_GE(children.size(), 4u);
  EXPECT_EQ(GetSidebarPinnedTabsView(), children.front())
      << "pinned tabs must be the first thing in the sidebar";
  EXPECT_EQ(GetSidebarSettingsButton(), children.back())
      << "the settings gear must be the last";

  const auto index_of = [&children](const views::View* view) {
    return std::ranges::find(children, view) - children.begin();
  };
  EXPECT_LT(index_of(GetSidebarBottomSeparator()),
            index_of(GetSidebarItemsScrollViewAsView()))
      << "the separator divides the pinned tabs from the bottom block";
  EXPECT_LT(index_of(GetSidebarItemsScrollViewAsView()),
            index_of(GetSidebarSettingsButton()));

  EXPECT_FALSE(SidebarHasAddButton())
      << "the add button must not be built at all";
  EXPECT_FALSE(control->context_menu_controller())
      << "the sidebar itself must not offer a context menu";

  // The accessor hands back a raw_ptr, so take it as one.
  auto items = GetSidebarItemsContentsView(controller());
  ASSERT_TRUE(items);
  EXPECT_FALSE(items->context_menu_controller());
  for (views::View* item : items->children()) {
    EXPECT_FALSE(item->context_menu_controller())
        << "a legacy sidebar button must not offer a context menu";
    EXPECT_FALSE(item->drag_controller())
        << "legacy sidebar buttons must not be draggable";
  }
}

}  // namespace sidebar
