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
// growser (#95): `extra_domains` are the ones fetched from our backend
// since the build. They are added to the bundled list, never substituted
// for it - a truncated or hostile payload can only widen what already
// works, never take away a domain the user relies on.
base::ListValue GetTrustAnchorsPrefDefault(
    const base::ListValue* extra_domains = nullptr);

}  // namespace growser_ru_trust

#endif  // BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_
