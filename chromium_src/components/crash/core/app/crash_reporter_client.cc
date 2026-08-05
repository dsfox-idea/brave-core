/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(OFFICIAL_BUILD)
#define BRAVE_CRASH_REPORTER_CLIENT_GET_UPLOAD_URL \
  return "https://cr.brave.com";
#else
// growser (#33): non-official builds upload Crashpad minidumps to our
// self-hosted GlitchTip (Sentry-protocol minidump endpoint). The DSN public
// key is public by Sentry design (safely embedded in client binaries, like
// Brave's cr.brave.com); project id 1 = "Growser Desktop". Consent is forced
// true in chrome_crash_reporter_client.cc::GetCollectStatsConsent for the
// same non-official case (see the Brave patch on that file).
#define BRAVE_CRASH_REPORTER_CLIENT_GET_UPLOAD_URL                         \
  return "https://growser-crashes.humans.top/api/1/minidump/"              \
         "?sentry_key=fb00160c-91d2-4f4d-8548-ebe69254bd61";
#endif

#include <components/crash/core/app/crash_reporter_client.cc>
#undef BRAVE_CRASH_REPORTER_CLIENT_GET_UPLOAD_URL
