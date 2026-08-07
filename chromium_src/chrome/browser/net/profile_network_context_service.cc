/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/growser_ru_trust/ru_trust_anchors.h"

// growser: доверие корню НУЦ Минцифры, ограниченное белым списком доменов
// (growser#36). Регистрируем как ЗНАЧЕНИЕ ПО УМОЛЧАНИЮ того же pref, который
// заполняет enterprise-политика CACertificatesWithConstraints — политика при
// необходимости перекрывает наш дефолт штатным образом.
#define BRAVE_REGISTER_CA_CERTIFICATES_WITH_CONSTRAINTS_PREF          \
  registry->RegisterListPref(prefs::kCACertificatesWithConstraints,   \
                             growser_ru_trust::GetTrustAnchorsPrefDefault());

static const char* kBraveCTExcludedHosts[] = {
    // Critical endpoints that shouldn't require SCTs so they always work
    "updates.bravesoftware.com",
    "updates-cdn.bravesoftware.com",
    "usage-ping.brave.com",
    // Test host for manual testing
    "sct-exempted.bravesoftware.com",
};

#define BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY \
  for (const auto* host : kBraveCTExcludedHosts) {          \
    excluded.push_back(host);                               \
  }

#include <chrome/browser/net/profile_network_context_service.cc>
#undef BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY
#undef BRAVE_REGISTER_CA_CERTIFICATES_WITH_CONSTRAINTS_PREF
