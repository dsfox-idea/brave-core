/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

import android.content.Context;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.suggestions.tile.BraveTileView;

/**
 * growser#392: with background images off the new tab page is the desktop's flat theme colour,
 * and the tiles sit on it directly instead of on the dark slab made for photos.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveNtpSolidBackgroundTest {
    private static int colorOf(Context context) {
        return context.getColor(R.color.ntp_solid_background) & 0xffffff;
    }

    @Test
    @Config(qualifiers = "notnight")
    public void lightThemeIsNearWhite() {
        assertEquals(0xf4f4f5, colorOf(RuntimeEnvironment.getApplication()));
    }

    @Test
    @Config(qualifiers = "night")
    public void darkThemeIsDarkGrey() {
        assertEquals(0x202124, colorOf(RuntimeEnvironment.getApplication()));
    }

    // growser#393: the icon plate stands off the flat page, so the tile keeps its shape.
    private static int plateOf(Context context) {
        return BraveTileView.solidBackgroundIconPlate(context).getColor().getDefaultColor()
                & 0xffffff;
    }

    @Test
    @Config(qualifiers = "notnight")
    public void lightIconPlateIsDarkerThanThePage() {
        Context context = RuntimeEnvironment.getApplication();
        assertEquals(0xe3e3e6, plateOf(context));
        assertNotEquals(colorOf(context), plateOf(context));
    }

    @Test
    @Config(qualifiers = "night")
    public void darkIconPlateIsLighterThanThePage() {
        Context context = RuntimeEnvironment.getApplication();
        assertEquals(0x3c3d41, plateOf(context));
        assertNotEquals(colorOf(context), plateOf(context));
    }

    @Test
    public void tilesKeepTheirSlabOnlyOverAPhoto() {
        assertEquals(R.drawable.rounded_dark_bg_alpha, BraveNtpAdapter.tilesBackground(true));
        assertEquals(0, BraveNtpAdapter.tilesBackground(false));
    }
}
