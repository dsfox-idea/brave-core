/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.minidump_uploader.util;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.net.HttpURLConnection;

@NullMarked
public class BraveHttpURLConnectionFactoryImpl extends HttpURLConnectionFactoryImpl {
    // Growser-315: Brave sent every upload to cr.brave.com here, whatever URL
    // the uploader asked for. The uploader's URL is ours (MinidumpUploader's
    // sCrashUrlString, patched), so the connection goes where it is asked to.
    @Override
    public @Nullable HttpURLConnection createHttpURLConnection(String url) {
        return super.createHttpURLConnection(url);
    }
}
