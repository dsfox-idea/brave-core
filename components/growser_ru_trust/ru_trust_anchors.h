/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_
#define BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_

#include "base/values.h"

namespace growser_ru_trust {

// The default value of the kCACertificatesWithConstraints pref: the Russian
// Ministry of Digital Development root ("Russian Trusted Root CA"), trusted
// only for an allowlist of domains. Outside that list the root vouches for
// nothing, so it cannot be used to impersonate an arbitrary site (growser#36).
//
// The shape is the one the CACertificatesWithConstraints enterprise policy
// uses:
// [{"certificate": <base64 DER>, "constraints": {"permitted_dns_names": [...]}}]
base::ListValue GetTrustAnchorsPrefDefault();

}  // namespace growser_ru_trust

#endif  // BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_
