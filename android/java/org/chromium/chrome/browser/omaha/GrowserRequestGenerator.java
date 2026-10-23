/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omaha;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.ServiceImpl;

import java.util.regex.Pattern;

/**
 * growser#378: the update check of the Android beta handed out from growser.org/android.
 *
 * <p>Chromium's Omaha client does nothing until a RequestGenerator is registered. This one sends
 * the check to our backend, which answers from the version growser.org/android serves. The
 * request carries the app id, the installed version and whether this is the first run - none of
 * the OS version, architecture, language, install age or device model the stock request has.
 */
@NullMarked
@ServiceImpl(RequestGenerator.class)
public class GrowserRequestGenerator extends RequestGenerator {
    static final String APP_ID = "org.growser";
    static final String SERVER_URL = "https://backend.growser.org/android/update";
    private static final Pattern VERSION = Pattern.compile("[0-9]+(\\.[0-9]+){3}");
    private static final String UNKNOWN_VERSION = "0.0.0.0";

    @Override
    public String generateXML(
            String sessionID,
            String versionName,
            long installAge,
            int lastCheckDate,
            RequestData data) {
        String version =
                VERSION.matcher(versionName).matches() ? versionName : UNKNOWN_VERSION;
        String check =
                data.isSendInstallEvent()
                        ? "<event eventtype=\"2\" eventresult=\"1\"/>"
                        : "<updatecheck/><ping active=\"1\"/>";
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><request protocol=\"3.0\">"
                + "<app appid=\""
                + APP_ID
                + "\" version=\""
                + version
                + "\">"
                + check
                + "</app></request>";
    }

    @Override
    public String getServerUrl() {
        return SERVER_URL;
    }

    @Override
    protected String getAppIdHandset() {
        return APP_ID;
    }

    @Override
    protected String getAppIdTablet() {
        return APP_ID;
    }

    @Override
    protected boolean getLayoutIsTablet() {
        return false;
    }

    @Override
    protected String getBrand() {
        return "";
    }

    @Override
    protected String getClient() {
        return "";
    }
}
