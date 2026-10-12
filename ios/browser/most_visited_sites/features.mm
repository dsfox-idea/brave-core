// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/most_visited_sites/features.h"

#include "base/feature.h"

namespace topsites::features {

// Growser-310: on, so the new tab page can show the most visited sites (the
// default mode below), as Android does.
BASE_FEATURE(kTopsitesEnabled, base::FEATURE_ENABLED_BY_DEFAULT);
}
