/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.browsing_data;

import android.os.Bundle;
import android.text.SpannableString;
import android.view.View;

import androidx.preference.PreferenceGroupAdapter;
import androidx.preference.PreferenceScreen;

import org.chromium.base.Callback;
import org.chromium.build.annotations.NonNull;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveAdsNativeHelper;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveSettingsPreferenceGroupAdapter;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.browser_ui.settings.ClickableSpansTextMessagePreference;
import org.chromium.components.browser_ui.settings.SpinnerPreference;
import org.chromium.ui.text.ChromeClickableSpan;
import org.chromium.ui.text.SpanApplier;
import org.chromium.ui.text.SpanApplier.SpanInfo;

public class BraveClearBrowsingDataFragment extends ClearBrowsingDataFragment {
    ClearBrowsingDataCheckBoxPreference mClearAIChatDataCheckBoxPreference;

    @Override
    protected @NonNull PreferenceGroupAdapter onCreateAdapter(
            @NonNull PreferenceScreen preferenceScreen) {
        return new BraveSettingsPreferenceGroupAdapter(preferenceScreen);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        getPreferenceScreen()
                .addPreference(
                        BraveRewardsHelper.isRewardsEnabled()
                                ? buildResetBraveRewardsDataPref()
                                : buildClearBraveAdsDataPref());
    }

    private ClickableSpansTextMessagePreference buildResetBraveRewardsDataPref() {
        SpannableString resetBraveRewardsDataText =
                SpanApplier.applySpans(
                        getContext().getString(R.string.reset_brave_rewards_data),
                        new SpanInfo(
                                "<link1>",
                                "</link1>",
                                new ChromeClickableSpan(
                                        requireContext(), resetBraveRewardsDataCallback())));

        ClickableSpansTextMessagePreference resetBraveRewardsDataPref =
                new ClickableSpansTextMessagePreference(getContext(), null);
        resetBraveRewardsDataPref.setSummary(resetBraveRewardsDataText);
        return resetBraveRewardsDataPref;
    }

    private ClickableSpansTextMessagePreference buildClearBraveAdsDataPref() {
        SpannableString clearBraveAdsDataText =
                SpanApplier.applySpans(
                        getContext().getString(R.string.clear_brave_ads_data),
                        new SpanInfo(
                                "<link1>",
                                "</link1>",
                                new ChromeClickableSpan(
                                        requireContext(), clearBraveAdsDataCallback())));

        ClickableSpansTextMessagePreference clearBraveAdsDataPref =
                new ClickableSpansTextMessagePreference(getContext(), null);
        clearBraveAdsDataPref.setSummary(clearBraveAdsDataText);
        return clearBraveAdsDataPref;
    }

    private Callback<View> resetBraveRewardsDataCallback() {
        return (view) -> {
            try {
                TabUtils.openUrlInNewTab(false, BraveActivity.BRAVE_REWARDS_RESET_PAGE);
                TabUtils.bringChromeTabbedActivityToTheTop(BraveActivity.getBraveActivity());
            } catch (BraveActivity.BraveActivityNotFoundException e) {
            }
        };
    }

    private Callback<View> clearBraveAdsDataCallback() {
        return (view) -> {
            Profile profile = getProfile();
            if (profile != null) {
                BraveAdsNativeHelper.nativeClearData(profile);
            }

            if (getActivity() != null) {
                getActivity().finish();
            }
        };
    }

    @Override
    protected void onClearBrowsingData() {
        super.onClearBrowsingData();

        // Growser-270: Leo is out of the product, so there are no
        // conversations to clear.
    }
}
