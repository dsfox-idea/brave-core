// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/browser/android/safe_browsing/buildflags.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_stats/buildflags.h"
#include "chrome/android/chrome_jni_headers/BraveActivity_jni.h"

#if BUILDFLAG(ENABLE_BRAVE_STATS_UPDATER)  // Growser-261
#include "brave/browser/brave_stats/brave_stats_updater.h"
#endif

namespace chrome {
namespace android {

static void JNI_BraveActivity_RestartStatsUpdater(JNIEnv* env) {
  // Growser-261: the JNI entry stays (the generated natives table names it);
  // without the updater there is nothing to restart.
#if BUILDFLAG(ENABLE_BRAVE_STATS_UPDATER)
  g_brave_browser_process->brave_stats_updater()->Stop();
  g_brave_browser_process->brave_stats_updater()->Start();
#endif
}

static base::android::ScopedJavaLocalRef<jstring>
JNI_BraveActivity_GetSafeBrowsingApiKey(JNIEnv* env) {
  return base::android::ConvertUTF8ToJavaString(
      env, BUILDFLAG(SAFEBROWSING_API_KEY));
}

}  // namespace android
}  // namespace chrome

DEFINE_JNI(BraveActivity)
