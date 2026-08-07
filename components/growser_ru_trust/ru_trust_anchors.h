/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_
#define BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_

#include "base/values.h"

namespace growser_ru_trust {

// Значение по умолчанию для pref kCACertificatesWithConstraints: корень НУЦ
// Минцифры («Russian Trusted Root CA») с доверием, ограниченным белым списком
// доменов. Вне списка корень не может подтвердить ни один сертификат, поэтому
// он не даёт возможности подменить произвольный сайт (growser#36).
//
// Формат совпадает с enterprise-политикой CACertificatesWithConstraints:
// [{"certificate": <base64 DER>, "constraints": {"permitted_dns_names": [...]}}]
base::ListValue GetTrustAnchorsPrefDefault();

}  // namespace growser_ru_trust

#endif  // BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_ANCHORS_H_
