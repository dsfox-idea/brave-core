/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.ntp;

public interface OnBraveNtpListener {
    // Growser-272: updateNewsOptin, getFeed and loadNewContent left with Brave News.
    public void checkForBraveStats();
}
