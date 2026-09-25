/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.minidump_uploader;

import androidx.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.components.minidump_uploader.util.BraveHttpURLConnectionFactoryImpl;
import org.chromium.components.minidump_uploader.util.HttpURLConnectionFactory;

import java.net.HttpURLConnection;

/** Unittests for {@link BraveHttpURLConnectionFactoryImpl}. */
@RunWith(BaseRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class BraveHttpURLConnectionFactoryImplTest {
    @Test
    @SmallTest
    // Growser-315: the upload goes to the URL the uploader asks for.
    public void testUploadUrlIsTheOneAskedFor() {
        HttpURLConnectionFactory httpURLConnectionFactory = new BraveHttpURLConnectionFactoryImpl();
        String asked = "https://growser-crashes.humans.top/api/1/minidump/?sentry_key=k";
        HttpURLConnection connection = httpURLConnectionFactory.createHttpURLConnection(asked);
        Assert.assertEquals(asked, connection.getURL().toString());
    }
}
