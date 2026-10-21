/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.notifications.permissions;

import static org.junit.Assert.assertEquals;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.chrome.R;

/**
 * The notification permission rationale must not promise Rewards, which this browser does not
 * have (growser#351): it shows Chromium's message, not Brave's "earn tokens" one.
 */
@RunWith(BaseRobolectricTestRunner.class)
public class BraveNotificationPermissionRationaleDialogTest {
    @Test
    public void message_isChromiumsRationale() {
        Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
        activity.setTheme(R.style.Theme_BrowserUI_DayNight);
        View view =
                LayoutInflater.from(activity)
                        .inflate(R.layout.brave_notification_permission_rationale_dialog, null);

        TextView message = view.findViewById(R.id.notification_permission_rationale_message);

        assertEquals(
                activity.getString(R.string.notification_permission_rationale_dialog_message),
                message.getText().toString());
    }
}
