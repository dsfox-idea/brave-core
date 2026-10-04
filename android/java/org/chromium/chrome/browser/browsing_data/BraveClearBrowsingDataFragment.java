/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import android.view.View;

import androidx.preference.PreferenceGroupAdapter;
import androidx.preference.PreferenceScreen;

import org.chromium.build.annotations.NonNull;
import org.chromium.chrome.browser.settings.BottomInsetViewProvider;
import org.chromium.chrome.browser.settings.BraveSettingsPreferenceGroupAdapter;

public class BraveClearBrowsingDataFragment extends ClearBrowsingDataFragment
        implements BottomInsetViewProvider {
    ClearBrowsingDataCheckBoxPreference mClearAIChatDataCheckBoxPreference;

    @Override
    public View getBottomInsetView(View fragmentView) {
        // The delete button is outside the preference list.
        return fragmentView;
    }

    @Override
    protected @NonNull PreferenceGroupAdapter onCreateAdapter(
            @NonNull PreferenceScreen preferenceScreen) {
        return new BraveSettingsPreferenceGroupAdapter(preferenceScreen);
    }

    // Growser-271: the rewards reset and ads clear rows left with the feature.

    @Override
    protected void onClearBrowsingData() {
        super.onClearBrowsingData();

        // Growser-270: Leo is out of the product, so there are no
        // conversations to clear.
    }
}
