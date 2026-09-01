/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_control_view.h"

#include "base/check.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
// Growser-149: the add button is not created any more, but its code stays in
// the tree - the body below is kept intact behind an early return.
#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_pinned_tabs_view.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/flex_layout.h"

namespace {

using ShowSidebarOption = sidebar::SidebarService::ShowSidebarOption;

// To use bold font for title at index 0.
class ControlViewMenuModel : public ui::SimpleMenuModel {
 public:
  using SimpleMenuModel::SimpleMenuModel;
  ~ControlViewMenuModel() override = default;
  ControlViewMenuModel(const ControlViewMenuModel&) = delete;
  ControlViewMenuModel& operator=(const ControlViewMenuModel&) = delete;

  // ui::SimpleMenuModel overrides:
  const gfx::FontList* GetLabelFontListAt(size_t index) const override {
    if (GetTypeAt(index) == ui::MenuModel::TYPE_TITLE) {
      return &ui::ResourceBundle::GetSharedInstance().GetFontList(
          ui::ResourceBundle::BoldFont);
    }
    return SimpleMenuModel::GetLabelFontListAt(index);
  }
};

bool IsSidebarOnLeft(Browser* browser) {
  return !browser->GetProfile()->GetPrefs()->GetBoolean(
      prefs::kSidePanelHorizontalAlignment);
}

}  // namespace

SidebarControlView::SidebarControlView(Delegate* delegate,
                                       BraveBrowser* browser)
    : delegate_(delegate), browser_(browser) {
  // Don't follow RTL layout. Sidebar position is determined by its own setting.
  SetMirrored(false);
  // Growser-149: no context menu on the sidebar's legacy surface.

  AddChildViews();
  UpdateItemAddButtonState();
  UpdateSettingsButtonState();

  sidebar_model_observed_.Observe(
      browser_->GetFeatures().sidebar_controller()->model());
  SetLayoutManager(std::make_unique<views::FlexLayout>())
      ->SetOrientation(views::LayoutOrientation::kVertical);
  SetBackground(views::CreateSolidBackground(kColorToolbar));
}

void SidebarControlView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateItemAddButtonState();
  UpdateSettingsButtonState();
}

void SidebarControlView::UpdateBorder() {
  // When rounded corners is on, contents and side panel already have an
  // inward margin. Without adjustment the gap between the control view and
  // the content would be double that margin. Use a negative inset on the
  // content-facing side to overlap into that existing margin, keeping visual
  // spacing tight. When rounded corners is off the margin is 0, so
  // overlap is 0.
  const int overlap =
      -BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser_);
  SetBorder(views::CreateEmptyBorder(gfx::Insets::TLBR(
      0, sidebar_on_left_ ? 0 : overlap, 0, sidebar_on_left_ ? overlap : 0)));
}

SidebarControlView::~SidebarControlView() = default;

// Growser-149: no context menu on the sidebar's legacy surface. The body is
// kept, unreachable, because nothing sets this view as a context menu
// controller any more.
void SidebarControlView::ShowContextMenuForViewImpl(
    views::View* source,
    const gfx::Point& point,
    ui::mojom::MenuSourceType source_type) {
  if (context_menu_runner_ && context_menu_runner_->IsRunning()) {
    return;
  }

  context_menu_model_ = std::make_unique<ControlViewMenuModel>(this);
  context_menu_model_->AddTitle(
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_TITLE));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowAlways),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_ALWAYS));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowOnMouseOver),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_MOUSEOVER));
  context_menu_model_->AddCheckItem(
      static_cast<int>(ShowSidebarOption::kShowNever),
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SHOW_OPTION_NEVER));
  context_menu_model_->AddSeparator(
      ui::MenuSeparatorType::BOTH_SIDE_PADDED_SEPARATOR);
  context_menu_model_->AddTitle(
      l10n_util::GetStringUTF16(IDS_SIDEBAR_MENU_MODEL_POSITION_OPTION_TITLE));
  context_menu_model_->AddItemWithStringId(
      IDC_SIDEBAR_TOGGLE_POSITION,
      IsSidebarOnLeft(browser_)
          ? IDS_SIDEBAR_MENU_MODEL_POSITION_MOVE_TO_RIGHT_OPTION
          : IDS_SIDEBAR_MENU_MODEL_POSITION_MOVE_TO_LEFT_OPTION);
  context_menu_runner_ = std::make_unique<views::MenuRunner>(
      context_menu_model_.get(), views::MenuRunner::CONTEXT_MENU);
  context_menu_runner_->RunMenuAt(
      source->GetWidget(), nullptr, gfx::Rect(point, gfx::Size()),
      views::MenuAnchorPosition::kTopLeft, source_type);
}

void SidebarControlView::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == IDC_SIDEBAR_TOGGLE_POSITION) {
    browser_->command_controller()->ExecuteCommand(command_id);
    return;
  }
  auto* service =
      sidebar::SidebarServiceFactory::GetForProfile(browser_->GetProfile());
  service->SetSidebarShowOption(static_cast<ShowSidebarOption>(command_id));
}

bool SidebarControlView::IsCommandIdChecked(int command_id) const {
  const auto* service =
      sidebar::SidebarServiceFactory::GetForProfile(browser_->GetProfile());
  return static_cast<ShowSidebarOption>(command_id) ==
         service->GetSidebarShowOption();
}

void SidebarControlView::MenuClosed(ui::SimpleMenuModel* source) {
  delegate_->MenuClosed();
}

void SidebarControlView::OnItemAdded(const sidebar::SidebarItem& item,
                                     size_t index,
                                     bool user_gesture) {
  UpdateItemAddButtonState();
}

void SidebarControlView::OnItemRemoved(size_t index) {
  UpdateItemAddButtonState();
}

void SidebarControlView::AddChildViews() {
  // Growser-149: pinned tabs first, at the very top of the sidebar. What is
  // left of the legacy sidebar - bookmarks, reading list - sits at the bottom
  // with the settings gear, behind a separator.
  sidebar_pinned_tabs_view_ =
      AddChildView(std::make_unique<SidebarPinnedTabsView>(browser_));
  sidebar_pinned_tabs_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero)
          .WithOrder(2));

  // Pushes the bottom block down. Everything in that block is inflexible, so
  // it always keeps its space and the pinned tabs take whatever is left.
  auto* spacer = AddChildView(std::make_unique<views::View>());
  spacer->SetEnabled(false);
  spacer->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero,
                               views::MaximumFlexSizeRule::kUnbounded)
          .WithOrder(1));

  // Growser-149: the divider the pinned block used to draw above itself now
  // belongs here - it separates the pinned tabs from what is below them.
  bottom_separator_ = AddChildView(std::make_unique<views::Separator>());
  bottom_separator_->SetColorId(kColorSidebarSeparator);
  bottom_separator_->SetProperty(
      views::kMarginsKey,
      gfx::Insets::VH(SidebarButtonView::kMargin, SidebarButtonView::kMargin));

  sidebar_items_view_ =
      AddChildView(std::make_unique<SidebarItemsScrollView>(browser_));
  sidebar_items_view_->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToZero)
          .WithOrder(3));

  // Growser-149: the add button is gone. Nothing creates it any more, and
  // SidebarItemAddButton stays in the tree switched off rather than deleted -
  // a Chromium bump should not turn this into a merge conflict, and putting it
  // back is then a one-line change.

  sidebar_settings_view_ = AddChildView(std::make_unique<SidebarButtonView>(
      l10n_util::GetStringUTF16(IDS_SIDEBAR_SETTINGS_BUTTON_TOOLTIP)));

  sidebar_settings_view_->SetCallback(
      base::BindRepeating(&SidebarControlView::OnButtonPressed,
                          base::Unretained(this), sidebar_settings_view_));
}

void SidebarControlView::OnButtonPressed(views::View* view) {
  if (view == sidebar_settings_view_) {
    ShowSingletonTabOverwritingNTP(
        browser_,
        GURL("chrome://settings?search=" +
             l10n_util::GetStringUTF8(
                 IDS_SETTINGS_APPEARNCE_SETTINGS_SIDEBAR_PART_TITLE)));
  }
}

void SidebarControlView::Update() {
  UpdateItemAddButtonState();
  sidebar_items_view_->Update();
}

void SidebarControlView::UpdateItemAddButtonState() {
  // Growser-149: there is no add button any more. Kept as a no-op so the call
  // sites (item added/removed, theme changes) stay untouched.
  if (!sidebar_item_add_view_) {
    return;
  }

  // Determine add button enabled state.
  bool should_enable = true;
  if (browser_->GetFeatures()
          .sidebar_controller()
          ->model()
          ->IsSidebarHasAllBuiltInItems() &&
      !sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    should_enable = false;
  }
  sidebar_item_add_view_->SetEnabled(should_enable);
}

void SidebarControlView::UpdateSettingsButtonState() {
  DCHECK(sidebar_settings_view_);
  sidebar_settings_view_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon, kColorSidebarButtonBase,
                                     SidebarButtonView::kDefaultIconSize));
  sidebar_settings_view_->SetImageModel(
      views::Button::STATE_PRESSED,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon,
                                     kColorSidebarButtonPressed,
                                     SidebarButtonView::kDefaultIconSize));
  sidebar_settings_view_->SetImageModel(
      views::Button::STATE_DISABLED,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon,
                                     kColorToolbarButtonIconInactive,
                                     SidebarButtonView::kDefaultIconSize));
}

bool SidebarControlView::IsItemReorderingInProgress() const {
  return sidebar_items_view_->IsItemReorderingInProgress();
}

bool SidebarControlView::IsBubbleWidgetVisible() const {
  if (context_menu_runner_ && context_menu_runner_->IsRunning()) {
    return true;
  }

  // Growser-149: no add button, so no add bubble to be visible.

  if (sidebar_items_view_->IsBubbleVisible()) {
    return true;
  }

  return false;
}

void SidebarControlView::SetSidebarOnLeft(bool sidebar_on_left) {
  sidebar_on_left_ = sidebar_on_left;
  UpdateBorder();
}

BEGIN_METADATA(SidebarControlView)
END_METADATA
