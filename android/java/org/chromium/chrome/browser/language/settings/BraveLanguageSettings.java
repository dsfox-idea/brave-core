/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.language.settings;

import android.content.Context;
import android.os.Bundle;

import androidx.preference.PreferenceCategory;

import org.chromium.components.browser_ui.settings.search.BaseSearchIndexProvider;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

public class BraveLanguageSettings extends LanguageSettings {
    static final String TRANSLATION_SETTINGS_SECTION = "translation_settings_section";
    static final String APP_LANGUAGE_SECTION = "app_language_section";

    public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
            new BaseSearchIndexProvider(
                    BraveLanguageSettings.class.getName(),
                    org.chromium.chrome.browser.language.R.xml.languages_detailed_preferences) {

                @Override
                public void updateDynamicPreferences(Context context, SettingsIndexData indexData) {
                    String frag = BraveLanguageSettings.class.getName();
                    // Brave removes translation_settings_section entirely. Each child must be
                    // removed explicitly — the search index parent-child links are based on
                    // android:fragment navigation, not PreferenceCategory containment.
                    indexData.removeEntryForKey(frag, TRANSLATION_SETTINGS_SECTION);
                    indexData.removeEntryForKey(frag, LanguageSettings.TRANSLATE_SWITCH_KEY);
                    indexData.removeEntryForKey(
                            frag, LanguageSettings.TRANSLATION_ADVANCED_SECTION);
                    indexData.removeEntryForKey(frag, LanguageSettings.TARGET_LANGUAGE_KEY);
                    indexData.removeEntryForKey(frag, LanguageSettings.ALWAYS_LANGUAGES_KEY);
                    indexData.removeEntryForKey(frag, LanguageSettings.NEVER_LANGUAGES_KEY);
                    // Remove preferences with no title that would appear as blank search results.
                    indexData.removeEntryForKey(frag, LanguageSettings.APP_LANGUAGE_PREFERENCE_KEY);
                    indexData.removeEntryForKey(frag, LanguageSettings.CONTENT_LANGUAGES_KEY);
                    // Growser-338: no Brave Translate toggle to index - translation is
                    // off on Android (#331), so the row it created switched nothing.
                }
            };

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);

        PreferenceCategory translateSwitch =
                (PreferenceCategory) findPreference(TRANSLATION_SETTINGS_SECTION);
        if (translateSwitch != null) {
            getPreferenceScreen().removePreference(translateSwitch);
        }

        // Growser-338: Brave adds a "Use Brave Translate" switch to the app
        // language section here. Translation is off on Android until #245
        // gives it a working path (#331), so the switch would promise a
        // feature this build does not have.
    }

    @Override
    public String getMainMenuKey() {
        return "brave_languages";
    }
}
