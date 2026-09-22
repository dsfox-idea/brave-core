/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/android/java/org/chromium/chrome/browser/search_engines/jni_headers/BraveSearchEnginePrefHelper_jni.h"

#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/brave_search_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace {
Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}
}  // namespace

// Growser-265: the pref below is registered only when the Brave Search
// default API feature is on (brave_profile_prefs.cc, `if
// (brave_search::IsDefaultAPIEnabled())`), and that feature is
// FEATURE_DISABLED_BY_DEFAULT. Brave's builds turn it on from a variations
// seed; we serve no variations (#41), so for us the default is the only
// answer there is - and BraveSearchEngineAdapter.getDSEShortName() asks this
// on EVERY start, from Java, unconditionally. Reading an unregistered pref
// is a CHECK, so the browser died with SIGTRAP before its first page.
//
// The reader consults the switch, rather than the pref being registered for
// a feature that is off: a pref nothing can set is not state, and an absent
// answer here means exactly what `false` means - do not take the search
// engine from native.
void JNI_BraveSearchEnginePrefHelper_SetFetchSEFromNative(JNIEnv* env,
                                                          jboolean value) {
  if (!brave_search::IsDefaultAPIEnabled()) {
    return;
  }
  GetOriginalProfile()->GetPrefs()->SetBoolean(
      brave_search::prefs::kFetchFromNative, value);
}

jboolean JNI_BraveSearchEnginePrefHelper_GetFetchSEFromNative(JNIEnv* env) {
  if (!brave_search::IsDefaultAPIEnabled()) {
    return false;
  }
  return GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_search::prefs::kFetchFromNative);
}

DEFINE_JNI(BraveSearchEnginePrefHelper)
