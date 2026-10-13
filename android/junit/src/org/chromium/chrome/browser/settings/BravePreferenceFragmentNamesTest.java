/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.content.Context;
import android.os.Bundle;
import android.text.TextUtils;

import androidx.fragment.app.Fragment;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.ContextUtils;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.components.browser_ui.settings.search.PreferenceParser;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Checks that every `android:fragment` in Brave's preference screens names a real Fragment.
 *
 * <p>Those names are plain strings: javac, gn and R8 all ignore them, so an upstream package move
 * leaves a screen that only fails when the user taps the row.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BravePreferenceFragmentNamesTest {
    /** A preference screen, plus a fragment that proves Brave's copy of it was the one parsed. */
    private static class Screen {
        public final int mXmlResId;
        public final @Nullable String mBraveOnlyFragment;

        Screen(int xmlResId) {
            this(xmlResId, null);
        }

        Screen(int xmlResId, @Nullable String braveOnlyFragment) {
            mXmlResId = xmlResId;
            mBraveOnlyFragment = braveOnlyFragment;
        }
    }

    private static final Screen[] BRAVE_PREFERENCE_SCREENS = {
        new Screen(R.xml.brave_appearance_preferences),
        // Growser-270: no Leo screens. Only the Leo fragments load them, and those are
        // compiled out with enable_ai_chat, so the screens cannot be reached.
        new Screen(R.xml.brave_main_preferences),
        new Screen(R.xml.brave_privacy_preferences),
        new Screen(R.xml.brave_search_engines_preferences),
        new Screen(R.xml.brave_tabs_and_tab_groups_preferences),
        // Growser-275: no wallet screen either - only the wallet's own fragment loads it.
        new Screen(
                org.chromium.components.browser_ui.site_settings.R.xml
                        .brave_site_settings_preferences),
        // Brave overrides upstream resources of the same name, so pin a fragment that only
        // Brave's copy has - otherwise the test could pass while parsing upstream's file.
        new Screen(
                R.xml.developer_preferences,
                "org.chromium.chrome.browser.settings.developer.BraveQAPreferences"),
        new Screen(
                R.xml.legal_information_preferences,
                "org.chromium.chrome.browser.settings.BraveLicensePreferences"),
    };

    // Growser-275: rows whose fragments left the build with the wallet. Their screens are
    // shared, so the rows stay in the XML and are removed when the screen is built:
    // BravePrivacySettings drops the decentralized DNS rows, and BraveSiteSettingsDelegate
    // reports the wallet as disabled so the connected-sites rows go too.
    private static final Set<String> COMPILED_OUT_FRAGMENTS =
            Set.of(
                    "org.chromium.chrome.browser.decentralized_dns.settings"
                            + ".UnstoppableDomainsSettingsFragment",
                    "org.chromium.chrome.browser.decentralized_dns.settings.ENSSettingsFragment",
                    "org.chromium.chrome.browser.decentralized_dns.settings.SnsSettingsFragment",
                    "org.chromium.chrome.browser.site_settings.BraveWalletEthereumConnectedSites",
                    "org.chromium.chrome.browser.site_settings.BraveWalletSolanaConnectedSites");

    @Test
    public void testFragmentAttributesNameRealFragments() throws Exception {
        Context context = ContextUtils.getApplicationContext();
        for (Screen screen : BRAVE_PREFERENCE_SCREENS) {
            String name = context.getResources().getResourceEntryName(screen.mXmlResId);
            List<String> fragments = parseFragments(context, screen.mXmlResId);
            Assert.assertFalse("Parsed no preferences out of " + name, fragments.isEmpty());

            if (screen.mBraveOnlyFragment != null) {
                Assert.assertTrue(
                        "Parsed upstream's "
                                + name
                                + " instead of Brave's, so this screen is not really covered",
                        fragments.contains(screen.mBraveOnlyFragment));
            }

            for (String fragment : fragments) {
                if (COMPILED_OUT_FRAGMENTS.contains(fragment)) {
                    continue;
                }
                Class<?> clazz;
                try {
                    // Loaded without initializing: static initializers of real settings fragments
                    // query feature flags, which throws when native is not up.
                    clazz =
                            Class.forName(
                                    fragment, /* initialize= */ false, getClass().getClassLoader());
                } catch (ClassNotFoundException e) {
                    throw new AssertionError(
                            name
                                    + " points android:fragment at "
                                    + fragment
                                    + ", which does not exist - it was most likely renamed or"
                                    + " moved to another package upstream",
                            e);
                }
                Assert.assertTrue(
                        name + " points android:fragment at the non-Fragment " + fragment,
                        Fragment.class.isAssignableFrom(clazz));
            }
        }
    }

    private static List<String> parseFragments(Context context, int xmlResId) throws Exception {
        List<String> fragments = new ArrayList<>();
        for (Bundle preference : PreferenceParser.parsePreferences(context, xmlResId)) {
            String fragment = preference.getString(PreferenceParser.METADATA_FRAGMENT);
            if (!TextUtils.isEmpty(fragment)) fragments.add(fragment);
        }
        return fragments;
    }
}
