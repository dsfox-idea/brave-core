/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import org.jni_zero.CalledByNative;

import org.chromium.build.annotations.NullMarked;

/**
 * Opens the Brave Origin settings screen when a purchase is first detected (from native for web
 * purchases, or from Java when a Play Store purchase is auto-restored), so the user can restart to
 * apply the newly enforced policies.
 */
@NullMarked
public class BraveOriginSettingsLauncherHelper {
    // Growser-317: the Brave Origin settings screen left with Play Billing. Native
    // still calls this after a web purchase, so the entry point stays and does
    // nothing.
    @CalledByNative
    public static void showOriginSettingsForRestart() {}
}
