/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omaha;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.ServiceLoaderUtil;
import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.browser.omaha.OmahaBase.VersionConfig;

import java.net.HttpURLConnection;

/** growser#378: the Android beta's update check, both ends of the exchange. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class GrowserRequestGeneratorTest {
    // What deploy/growser-backend/worker.js answers on 2026-10-23 12:00 UTC when
    // growser.org/android/version says 156.26.1022.0; test-worker.mjs asserts the same strings.
    private static final String UPDATE_ANSWER =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\" server=\"prod\">"
                    + "<daystart elapsed_seconds=\"43200\" elapsed_days=\"7235\"/>"
                    + "<app appid=\"org.growser\" status=\"ok\"><updatecheck status=\"ok\"><urls>"
                    + "<url codebase=\"https://growser.org/android/Growser.apk\"/></urls>"
                    + "<manifest version=\"156.26.1022.0\"/></updatecheck><ping status=\"ok\"/>"
                    + "</app></response>";
    private static final String NO_UPDATE_ANSWER =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\" server=\"prod\">"
                    + "<daystart elapsed_seconds=\"43200\" elapsed_days=\"7235\"/>"
                    + "<app appid=\"org.growser\" status=\"ok\"><updatecheck status=\"noupdate\"/>"
                    + "<ping status=\"ok\"/></app></response>";
    private static final String INSTALL_ANSWER =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?><response protocol=\"3.0\" server=\"prod\">"
                    + "<daystart elapsed_seconds=\"43200\" elapsed_days=\"7235\"/>"
                    + "<app appid=\"org.growser\" status=\"ok\"><event status=\"ok\"/></app>"
                    + "</response>";

    private static String request(String version, boolean installEvent) {
        return new GrowserRequestGenerator()
                .generateXML(
                        "session-id",
                        version,
                        /* installAge= */ 42,
                        /* lastCheckDate= */ 7234,
                        new RequestData(installEvent, 0, "request-id", "organic"));
    }

    @Test
    public void testOmahaFindsTheGenerator() {
        assertTrue(
                ServiceLoaderUtil.maybeCreate(RequestGenerator.class)
                        instanceof GrowserRequestGenerator);
    }

    /** OmahaBase as the browser builds it, with only what createConnection() reads. */
    private static final class Delegate extends OmahaDelegate {
        @Override
        boolean isInSystemImage() {
            return false;
        }

        @Override
        ExponentialBackoffScheduler getScheduler() {
            throw new AssertionError("not used by createConnection()");
        }

        @Override
        String generateUUID() {
            return "uuid";
        }

        @Override
        boolean isChromeBeingUsed() {
            return true;
        }

        @Override
        void scheduleService(long currentTimestampMs, long nextTimestampMs) {}

        @Override
        RequestGenerator createRequestGenerator() {
            return new GrowserRequestGenerator();
        }
    }

    @Test
    public void testTheConnectionNamesNeitherTheOsNorTheDevice() throws Exception {
        HttpURLConnection connection = new OmahaBase(new Delegate()).createConnection();
        assertEquals("https://backend.growser.org/android/update", connection.getURL().toString());
        assertEquals("Growser", connection.getRequestProperty("User-Agent"));
    }

    @Test
    public void testTheCheckGoesToOurBackend() {
        assertEquals(
                "https://backend.growser.org/android/update",
                new GrowserRequestGenerator().getServerUrl());
    }

    @Test
    public void testAnUpdateCheckCarriesOnlyTheAppAndItsVersion() {
        assertEquals(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?><request protocol=\"3.0\">"
                        + "<app appid=\"org.growser\" version=\"156.26.1021.9\">"
                        + "<updatecheck/><ping active=\"1\"/></app></request>",
                request("156.26.1021.9", false));
    }

    @Test
    public void testTheFirstRunSendsTheInstallEventInstead() {
        assertEquals(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?><request protocol=\"3.0\">"
                        + "<app appid=\"org.growser\" version=\"156.26.1021.9\">"
                        + "<event eventtype=\"2\" eventresult=\"1\"/></app></request>",
                request("156.26.1021.9", true));
    }

    @Test
    public void testAVersionNameThatIsNotAVersionIsNotSent() {
        assertTrue(request("1\"/><x a=\"", false).contains("version=\"0.0.0.0\""));
    }

    @Test
    public void testTheBackendAnswerNamesTheVersionAndTheApk() throws Exception {
        VersionConfig config =
                new ResponseParser(GrowserRequestGenerator.APP_ID, false)
                        .parseResponse(UPDATE_ANSWER);
        assertEquals("156.26.1022.0", config.latestVersion);
        assertEquals("https://growser.org/android/Growser.apk", config.downloadUrl);
        assertEquals("ok", config.updateStatus);
    }

    @Test
    public void testNoPublishedVersionIsNoUpdate() throws Exception {
        VersionConfig config =
                new ResponseParser(GrowserRequestGenerator.APP_ID, false)
                        .parseResponse(NO_UPDATE_ANSWER);
        assertEquals("noupdate", config.updateStatus);
        assertNull(config.latestVersion);
    }

    @Test
    public void testTheInstallEventAnswerIsAccepted() throws Exception {
        VersionConfig config =
                new ResponseParser(GrowserRequestGenerator.APP_ID, true)
                        .parseResponse(INSTALL_ANSWER);
        assertNull(config.latestVersion);
    }
}
