/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import android.app.Activity;

import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.edge_to_edge.EdgeToEdgeController;
import org.chromium.chrome.browser.ui.native_page.NativePageHost;

/** Brave's extension for IncognitoNewTabPage to add policy checks. */
@NullMarked
public class BraveIncognitoNewTabPage extends IncognitoNewTabPage {

    public BraveIncognitoNewTabPage(
            Activity activity,
            NativePageHost host,
            Profile profile,
            MonotonicObservableSupplier<EdgeToEdgeController> edgeToEdgeControllerSupplier) {
        super(activity, host, profile, edgeToEdgeControllerSupplier);

        // Growser-274: the VPN is out of the product, so the call to action
        // never shows and there is no policy state to pass.
        mIncognitoNewTabPageView.setVpnDisabledByPolicy(true);
    }
}
