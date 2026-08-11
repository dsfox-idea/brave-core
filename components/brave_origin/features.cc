/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/features.h"

#include "base/feature_list.h"

namespace brave_origin::features {

// growser (#78): Brave Origin is off. It is a paid edition of Brave, and with
// the feature on our settings offered to sell it: a "Growser Origin" block on
// the System page headed "Customize your browser while still supporting
// Growser", describing a paid product that does not exist, with Buy now and
// Restore purchase buttons and a link to Brave's site.
//
// Nothing here can be bought and nothing would be restored - the purchase flow
// is Brave's account service, which a fork has no account with. Rebranding the
// strings made it worse rather than better: it read as our own offer.
//
// This also takes down growser://settings/braveOrigin and the onboarding view,
// both of which are gated on the same feature.
BASE_FEATURE(kBraveOrigin, base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_origin::features
