/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/safe_browsing/features.h"

#include "base/feature_list.h"

namespace safe_browsing {
namespace features {

// Growser-303: off. Android Safe Browsing is Google Play Services' SafetyNet
// lookup with a key we do not have, so it sent every URL to Play Services and
// failed open; our Worker cannot serve it (Android has no local V4 database).
BASE_FEATURE(kBraveAndroidSafeBrowsing,
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
}  // namespace safe_browsing
