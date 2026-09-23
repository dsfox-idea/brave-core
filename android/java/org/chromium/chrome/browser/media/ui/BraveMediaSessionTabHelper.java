/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.media.ui;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.tab.Tab;

@NullMarked
public class BraveMediaSessionTabHelper extends MediaSessionTabHelper {
    /** Will be deleted in bytecode, value from the parent class will be used instead. */
    private @Nullable Tab mTab;

    BraveMediaSessionTabHelper(Tab tab) {
        super(tab);
    }

    // Growser-268: Brave Talk is out of the product, so talk.brave.com no longer
    // gets a microphone notification of its own - the three overrides that gave
    // it one are gone.
}
