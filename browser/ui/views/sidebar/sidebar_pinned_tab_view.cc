/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_pinned_tab_view.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "cc/paint/paint_flags.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_provider.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/view.h"

namespace {

// The tab strip's badge, at its numbers: a 16px disc carrying a 12px container
// icon (BraveTab::SmallAccentIconView).
constexpr int kBadgeSize = 16;
constexpr int kBadgeIconSize = 12;

// The accent outline. The tab strip strokes the tab's own rounded-rectangle
// shape, so the entry gets a rounded rectangle too rather than a circle - a
// ring reads as a different thing standing next to the strip.
constexpr float kOutlineStrokeWidth = 1.0f;
constexpr float kOutlineCornerRadius = 8.0f;

// Growser-150: the light on the sidebar's right edge for the tab the user is
// looking at. The bar is as tall as the favicon; the glow reaches left from it
// and is fully transparent before the icon's left edge, so it fades out
// instead of ending at one.
constexpr int kActiveBarWidth = 3;
constexpr int kActiveGlowWidth = 26;
constexpr int kActiveGlowVerticalPadding = 4;
constexpr SkAlpha kActiveGlowAlpha = 0x4D;

}  // namespace

// Everything accent-coloured lives here, on its own layer, and that is the
// point: ConfigureInkDrop puts the button's ink drop in LayerRegion::kAbove, so
// anything painted onto the button's canvas ends up *under* the hover and
// activation highlight - which is what made the badge look like it was behind
// the favicon and washed the entry out after a hover. The tab strip has the
// same problem and answers it the same way: BraveTab::SmallAccentIconView is a
// child that paints to a layer.
class SidebarPinnedTabView::AccentOverlayView : public views::View {
  METADATA_HEADER(AccentOverlayView, views::View)

 public:
  AccentOverlayView() {
    SetCanProcessEventsWithinSubtree(false);
    SetPaintToLayer();
    layer()->SetFillsBoundsOpaquely(false);
    SetVisible(false);
  }

  AccentOverlayView(const AccentOverlayView&) = delete;
  AccentOverlayView& operator=(const AccentOverlayView&) = delete;
  ~AccentOverlayView() override = default;

  void SetAccent(std::optional<TabAccentColors> colors, ui::ImageModel icon) {
    colors_ = colors;
    icon_ = std::move(icon);
    SetVisible(colors_.has_value());
    SchedulePaint();
  }

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override {
    if (!colors_.has_value()) {
      return;
    }

    gfx::RectF bounds(GetLocalBounds());
    bounds.Inset(kOutlineStrokeWidth / 2);

    cc::PaintFlags outline_flags;
    outline_flags.setAntiAlias(true);
    outline_flags.setStyle(cc::PaintFlags::kStroke_Style);
    outline_flags.setStrokeWidth(kOutlineStrokeWidth);
    outline_flags.setColor(colors_->border_color);
    canvas->DrawRoundRect(bounds, kOutlineCornerRadius, outline_flags);

    if (icon_.IsEmpty()) {
      return;
    }

    // Bottom-left, where BraveTab puts the small accent icon.
    const gfx::Point center(GetLocalBounds().x() + kBadgeSize / 2,
                            GetLocalBounds().bottom() - kBadgeSize / 2);

    cc::PaintFlags badge_flags;
    badge_flags.setAntiAlias(true);
    badge_flags.setStyle(cc::PaintFlags::kFill_Style);
    badge_flags.setColor(colors_->background_color);
    canvas->DrawCircle(center, kBadgeSize / 2, badge_flags);

    badge_flags.setStyle(cc::PaintFlags::kStroke_Style);
    badge_flags.setStrokeWidth(1);
    badge_flags.setColor(colors_->icon_border_color);
    canvas->DrawCircle(center, kBadgeSize / 2 - 0.5f, badge_flags);

    const gfx::ImageSkia image = icon_.Rasterize(GetColorProvider());
    canvas->DrawImageInt(image, 0, 0, image.width(), image.height(),
                         center.x() - kBadgeIconSize / 2,
                         center.y() - kBadgeIconSize / 2, kBadgeIconSize,
                         kBadgeIconSize, true);
  }

 private:
  std::optional<TabAccentColors> colors_;
  ui::ImageModel icon_;
};

BEGIN_METADATA(SidebarPinnedTabView, AccentOverlayView)
END_METADATA

SidebarPinnedTabView::SidebarPinnedTabView(
    const std::u16string& accessible_name)
    : SidebarItemView(accessible_name) {
  accent_overlay_ = AddChildView(std::make_unique<AccentOverlayView>());
}

SidebarPinnedTabView::~SidebarPinnedTabView() = default;

void SidebarPinnedTabView::SetContainerAccent(
    std::optional<TabAccentColors> colors,
    ui::ImageModel icon) {
  accent_colors_ = colors;
  has_accent_icon_ = !icon.IsEmpty();
  accent_overlay_->SetAccent(colors, std::move(icon));
}

bool SidebarPinnedTabView::IsAccentOnItsOwnLayer() const {
  return accent_overlay_->layer() != nullptr;
}

void SidebarPinnedTabView::SetActiveTab(bool active) {
  SetActiveState(active);

  if (is_active_tab_ == active) {
    return;
  }
  is_active_tab_ = active;
  SchedulePaint();
}

void SidebarPinnedTabView::OnPaintBackground(gfx::Canvas* canvas) {
  SidebarItemView::OnPaintBackground(canvas);

  const ui::ColorProvider* color_provider = GetColorProvider();
  if (!is_active_tab_ || !color_provider) {
    return;
  }

  // The container's colour when there is one, so the light and the container
  // accent read as one thing rather than two decorations.
  const SkColor color =
      accent_colors_.has_value()
          ? accent_colors_->border_color
          : color_provider->GetColor(kColorSidebarButtonPressed);

  const gfx::Rect contents = GetContentsBounds();
  const int bar_height = SidebarButtonView::kExternalIconSize;
  const int top = contents.CenterPoint().y() - bar_height / 2;
  const int right = GetLocalBounds().right();

  // The glow goes first: painted in the background, it ends up under the
  // favicon, which is where it belongs.
  const gfx::Rect glow(right - kActiveGlowWidth,
                       top - kActiveGlowVerticalPadding, kActiveGlowWidth,
                       bar_height + kActiveGlowVerticalPadding * 2);
  cc::PaintFlags glow_flags;
  glow_flags.setAntiAlias(true);
  glow_flags.setShader(gfx::CreateGradientShader(
      glow.right_center(), glow.left_center(),
      SkColorSetA(color, kActiveGlowAlpha),
      SkColorSetA(color, SK_AlphaTRANSPARENT)));
  canvas->DrawRect(glow, glow_flags);

  cc::PaintFlags bar_flags;
  bar_flags.setAntiAlias(true);
  bar_flags.setStyle(cc::PaintFlags::kFill_Style);
  bar_flags.setColor(color);
  canvas->DrawRoundRect(
      gfx::RectF(right - kActiveBarWidth, top, kActiveBarWidth, bar_height),
      kActiveBarWidth / 2.0f, bar_flags);
}

views::View* SidebarPinnedTabView::GetAccentOverlayForTesting() {
  return accent_overlay_;
}

void SidebarPinnedTabView::Layout(PassKey key) {
  LayoutSuperclass<SidebarItemView>(this);

  // Over the favicon, inside the button's own margins.
  accent_overlay_->SetBoundsRect(GetContentsBounds());
}

BEGIN_METADATA(SidebarPinnedTabView)
END_METADATA
