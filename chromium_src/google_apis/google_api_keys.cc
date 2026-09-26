/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <google_apis/google_api_keys.cc>

namespace google_apis {

void SetAPIKeyForTesting(const std::string& api_key) {
  GetApiKeyCacheInstance().set_api_key_for_testing(api_key);  // IN-TEST
}

bool BraveHasAPIKeyConfigured() {
#if BUILDFLAG(IS_ANDROID)
  // Growser-331: translation is off on Android. There is no on-device
  // translator there, so the page text would go to our backend, which has no
  // provider (#245). This is the gate TranslateManager reads: false turns off
  // the menu item, the automatic offer and IsAvailable together.
  return false;
#else
  // Google API key is not used in brave for translation service, always return
  // true for the API key check so the flow won't be blocked because of missing
  // keys.
  return true;
#endif
}

}  // namespace google_apis
