/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.GradientDrawable; // Growser-393
import android.util.AttributeSet;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.VisibleForTesting;
import androidx.core.widget.ImageViewCompat;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp.NtpUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.browser_ui.widget.tile.TileView;
import org.chromium.components.user_prefs.UserPrefs;

@NullMarked
public class BraveTileView extends TileView {
    private static final String TAG = "BraveTileView";

    public BraveTileView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    // Growser-393: the plate under a tile's icon when background images are off - one step
    // off ntp_solid_background, so the tile keeps its shape on the flat page.
    public static GradientDrawable solidBackgroundIconPlate(Context context) {
        GradientDrawable plate = new GradientDrawable();
        plate.setShape(GradientDrawable.OVAL);
        plate.setColor(context.getColor(R.color.ntp_tile_icon_plate));
        return plate;
    }

    @VisibleForTesting
    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
    }

    @Override
    public void togglePinnedShortcutBadge(boolean isVisible) {
        // In "Show shortcuts" mode every tile is a custom link, so the badge that distinguishes
        // manually-added shortcuts from frequently-visited ones is redundant on every tile. Only
        // show it in the mode where both kinds of tile can appear together.
        super.togglePinnedShortcutBadge(
                isVisible && NtpUtil.getTopSitesDisplayMode() != NtpUtil.TOP_SITES_MODE_SHORTCUTS);
    }

    @Override
    public void setTitle(String title, int titleLines) {
        super.setTitle(title, titleLines);
        if (ProfileManager.isInitialized()) {
            if (UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                    .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)) {
                TextView titleView = findViewById(R.id.tile_view_title);
                titleView.setTextAppearance(R.style.BraveSuggestionsTilesText);
                titleView.setShadowLayer(18, 0, 0, getContext().getColor(R.color.onboarding_black));
                // Add bottom padding to the text view
                int bottomPadding =
                        getResources()
                                .getDimensionPixelSize(
                                        R.dimen.tile_view_icon_background_margin_top_modern);
                titleView.setPadding(
                        titleView.getPaddingLeft(),
                        titleView.getPaddingTop(),
                        titleView.getPaddingRight(),
                        bottomPadding);

                // Get the text color from the TextView after TextAppearance is applied
                // and use it for the icon tint to match the text color
                ImageView pinnedShortcutBadgeView = findViewById(R.id.pinned_shortcut_badge);
                if (pinnedShortcutBadgeView != null) {
                    ColorStateList textColors = titleView.getTextColors();
                    ImageViewCompat.setImageTintList(pinnedShortcutBadgeView, textColors);
                }
            } else {
                // Growser-393: on the flat page the stock icon circle is the page colour.
                mIconBackgroundView.setBackground(solidBackgroundIconPlate(getContext()));
            }
        } else {
            Log.w(TAG, "Attempt to access profile before native initialization");
        }
    }
}
