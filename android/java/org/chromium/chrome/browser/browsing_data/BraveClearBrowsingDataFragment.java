/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import androidx.preference.PreferenceGroupAdapter;
import androidx.preference.PreferenceScreen;

import org.chromium.build.annotations.NonNull;
import org.chromium.chrome.browser.settings.BraveSettingsPreferenceGroupAdapter;

public class BraveClearBrowsingDataFragment extends ClearBrowsingDataFragment {
    ClearBrowsingDataCheckBoxPreference mClearAIChatDataCheckBoxPreference;

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
