/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_pinned_tab_view.h"

#include <utility>

#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "cc/paint/paint_flags.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/views/view.h"

namespace {

// The tab strip's badge, at the same numbers: a 16px circle carrying a 12px
// container icon (BraveTab::SmallAccentIconView).
constexpr int kBadgeSize = 16;
constexpr int kBadgeIconSize = 12;

// The button is square, so the accent outline is a circle around the favicon
// rather than the tab's shape. Inset by half the stroke so the stroke lands
// inside the view instead of being clipped.
constexpr float kOutlineStrokeWidth = 1.0f;

}  // namespace

SidebarPinnedTabView::SidebarPinnedTabView(
    const std::u16string& accessible_name)
    : SidebarItemView(accessible_name) {}

SidebarPinnedTabView::~SidebarPinnedTabView() = default;

void SidebarPinnedTabView::SetContainerAccent(
    std::optional<TabAccentColors> colors,
    ui::ImageModel icon) {
  if (accent_colors_.has_value() == colors.has_value() &&
      (!colors.has_value() ||
       (accent_colors_->border_color == colors->border_color &&
        accent_colors_->background_color == colors->background_color &&
        accent_colors_->icon_border_color == colors->icon_border_color)) &&
      accent_icon_ == icon) {
    return;
  }

  accent_colors_ = colors;
  accent_icon_ = std::move(icon);
  SchedulePaint();
}

void SidebarPinnedTabView::OnPaintBorder(gfx::Canvas* canvas) {
  SidebarItemView::OnPaintBorder(canvas);

  if (!accent_colors_.has_value()) {
    return;
  }

  gfx::RectF bounds(GetContentsBounds());
  bounds.Inset(kOutlineStrokeWidth / 2);
  const float radius = bounds.width() / 2;

  // Outline only. The tab strip draws the accent as a stroke around the tab
  // shape and leaves the inside alone - a filled disc here would read as a
  // different thing entirely, and would swallow the badge, which is painted in
  // the same background colour.
  cc::PaintFlags border_flags;
  border_flags.setAntiAlias(true);
  border_flags.setStyle(cc::PaintFlags::kStroke_Style);
  border_flags.setStrokeWidth(kOutlineStrokeWidth);
  border_flags.setColor(accent_colors_->border_color);
  canvas->DrawCircle(bounds.CenterPoint(), radius, border_flags);
}

void SidebarPinnedTabView::PaintChildren(const views::PaintInfo& paint_info) {
  SidebarItemView::PaintChildren(paint_info);

  // After the favicon, so the badge sits on top of it exactly as it does over
  // a tab's favicon in the strip.
  if (!accent_colors_.has_value() || accent_icon_.IsEmpty()) {
    return;
  }

  // Painting over children needs its own recorder - the tab container draws
  // its pinned/unpinned separator the same way.
  ui::PaintRecorder recorder(paint_info.context(), size(),
                             paint_info.paint_recording_scale_x(),
                             paint_info.paint_recording_scale_y(), nullptr);
  gfx::Canvas* canvas = recorder.canvas();

  // Bottom-left, where the tab strip puts it ("small accent icon on the
  // left-bottom corner of the tab", BraveTab::ShouldShowLargeAccentIcon).
  const gfx::Rect contents = GetContentsBounds();
  const gfx::Point center(contents.x() + kBadgeSize / 2,
                          contents.bottom() - kBadgeSize / 2);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(accent_colors_->background_color);
  canvas->DrawCircle(center, kBadgeSize / 2, flags);

  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setStrokeWidth(1);
  flags.setColor(accent_colors_->icon_border_color);
  canvas->DrawCircle(center, kBadgeSize / 2 - 0.5f, flags);

  const gfx::ImageSkia image = accent_icon_.Rasterize(GetColorProvider());
  canvas->DrawImageInt(image, 0, 0, image.width(), image.height(),
                       center.x() - kBadgeIconSize / 2,
                       center.y() - kBadgeIconSize / 2, kBadgeIconSize,
                       kBadgeIconSize, true);
}

BEGIN_METADATA(SidebarPinnedTabView)
END_METADATA
