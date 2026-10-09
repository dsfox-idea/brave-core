/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.privacy.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import androidx.test.core.app.ApplicationProvider;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.browsing_data.BraveClearBrowsingDataFragment;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.components.browser_ui.settings.search.SettingsIndexData;

import java.util.HashMap;

/**
 * The privacy page is Chromium's privacy_preferences.xml with Brave's rows on top, and settings
 * search has to find what the page shows (growser#327).
 */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BravePrivacySettingsSearchIndexTest {
    private static final String FRAGMENT = BravePrivacySettings.class.getName();

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private Profile mProfile;

    private SettingsIndexData mIndexData;

    @Before
    public void setUp() {
        mIndexData = SettingsIndexData.createInstance();
        BravePrivacySettings.SEARCH_INDEX_DATA_PROVIDER.initPreferenceXml(
                ApplicationProvider.getApplicationContext(),
                mProfile,
                mIndexData,
                new HashMap<>());
    }

    @After
    public void tearDown() {
        SettingsIndexData.reset();
    }

    @Test
    public void indexesChromiumRowsThePageShows() {
        assertNotNull(mIndexData.getEntryForKey(FRAGMENT, "safe_browsing"));
        assertNotNull(mIndexData.getEntryForKey(FRAGMENT, "clear_browsing_data"));
        assertNotNull(mIndexData.getEntryForKey(FRAGMENT, "secure_dns"));
        assertNotNull(mIndexData.getEntryForKey(FRAGMENT, "do_not_track"));
    }

    @Test
    public void keepsBraveRows() {
        assertNotNull(mIndexData.getEntryForKey(FRAGMENT, "clear_on_exit"));
    }

    @Test
    public void leavesOutChromiumRowsThePageRemoves() {
        assertNull(mIndexData.getEntryForKey(FRAGMENT, "password_leak_detection"));
        assertNull(mIndexData.getEntryForKey(FRAGMENT, "privacy_guide"));
        assertNull(mIndexData.getEntryForKey(FRAGMENT, "third_party_cookies"));
        assertNull(mIndexData.getEntryForKey(FRAGMENT, "preload_pages"));
        assertNull(mIndexData.getEntryForKey(FRAGMENT, "sync_and_services_link"));
    }

    @Test
    public void clearBrowsingDataOpensBravesFragment() {
        assertEquals(
                BraveClearBrowsingDataFragment.class.getName(),
                mIndexData.getEntryForKey(FRAGMENT, "clear_browsing_data").fragment);
    }
}
