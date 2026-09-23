// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/translate/features.h"

namespace brave::features {

BASE_FEATURE(kBraveTranslateEnabled,
             base::FEATURE_ENABLED_BY_DEFAULT);

// Growser-296: on by default - phrases are translated by Apple on the device
// and never leave it; only the script and its static files come over the
// network, through our backend.
BASE_FEATURE(kBraveAppleTranslateEnabled,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave::features
