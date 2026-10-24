/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omaha;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.ServiceImpl;
import org.chromium.net.ChromiumNetworkAdapter;
import org.chromium.net.NetworkTrafficAnnotationTag;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
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
    // growser#347: the auxiliary server on the same domain, for networks that
    // cannot reach backend.growser.org. It relays to the same Worker.
    static final String MIRROR_URL = "https://ru.growser.org/android/update";
    static final String HEALTH_ANSWER = "growser backend";
    private static final int HEALTH_TIMEOUT_MS = 5000;
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

    /**
     * growser#347: Omaha asks this on a background thread right before every check, so the
     * check goes to whichever of our two addresses answers now. Neither answering means the
     * phone is offline, and the check goes where it always did.
     */
    @Override
    public String getServerUrl() {
        if (answers("backend.growser.org") || !answers("ru.growser.org")) {
            return SERVER_URL;
        }
        return MIRROR_URL;
    }

    /** Whether {@code host}'s /healthz gives our backend's own answer. */
    boolean answers(String host) {
        NetworkTrafficAnnotationTag annotation =
                NetworkTrafficAnnotationTag.createComplete(
                        "growser_backend_health_android",
                        """
                        semantics {
                          sender: 'Growser backend route'
                          description:
                            'Asks whether the Growser backend answers on this network, and '
                            'when it does not, whether its auxiliary server does.'
                          trigger: 'Right before a beta update check.'
                          data: 'None. The request carries no identifiers.'
                          destination: OTHER
                        }
                        policy {
                          cookies_allowed: NO
                          setting: 'This feature cannot be disabled.'
                          policy_exception_justification: 'Not implemented.'
                        }\
                        """);
        HttpURLConnection connection = null;
        try {
            connection =
                    (HttpURLConnection)
                            ChromiumNetworkAdapter.openConnection(
                                    new URL("https://" + host + "/healthz"), annotation);
            connection.setConnectTimeout(HEALTH_TIMEOUT_MS);
            connection.setReadTimeout(HEALTH_TIMEOUT_MS);
            connection.setRequestProperty("User-Agent", "Growser");
            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) return false;
            byte[] body = new byte[HEALTH_ANSWER.length() + 1];
            int length = 0;
            try (InputStream stream = connection.getInputStream()) {
                int read;
                while (length < body.length
                        && (read = stream.read(body, length, body.length - length)) > 0) {
                    length += read;
                }
            }
            // The exact body, not any 200: a captive portal answers everything.
            return HEALTH_ANSWER.equals(new String(body, 0, length, StandardCharsets.UTF_8));
        } catch (IOException e) {
            return false;
        } finally {
            if (connection != null) connection.disconnect();
        }
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
