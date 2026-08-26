/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
// growser (#59): no host is exempt from Certificate Transparency.
//
// Brave exempts four of their own domains - their updater endpoints, their
// usage ping and a test host - so that those keep working even if CT logging
// misbehaves. For them that is a deliberate trade; for us it buys nothing and
// only removes a check, because we never call those hosts and the usage ping
// was compiled out entirely (#38). Carrying a weakened certificate check for
// another product's benefit is the whole of the bug.
//
// Our own backend sits behind Cloudflare with an ordinary, publicly logged
// certificate, so it needs no exemption either. The macro therefore expands to
// nothing rather than to a list of ours: an empty mechanism is easier to keep
// honest than an empty list.
#define BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY

// growser (#36): trust the Russian Ministry of Digital Development root
// ("Russian Trusted Root CA"), constrained to an allowlist of domains.
//
// Western CAs revoked the Russian banks' certificates in 2026, and the banks
// moved to this root - without it they do not open at all. It is registered as
// the DEFAULT VALUE of the very pref the CACertificatesWithConstraints
// enterprise policy writes, so an administrator still overrides us the
// ordinary way, and the constraints mean the root vouches for nothing outside
// the list - unlike installing it into the system store, where it would be
// valid for every domain there is.
// The fetched half lives in local state (#95); absent it, the bundled
// list stands alone, which is the state every fresh install starts in.
#define BRAVE_REGISTER_CA_CERTIFICATES_WITH_CONSTRAINTS_PREF        \
  registry->RegisterListPref(                                             \
      prefs::kCACertificatesWithConstraints,                              \
      growser_ru_trust::GetTrustAnchorsPrefDefault(                       \
          g_browser_process && g_browser_process->local_state()           \
              ? &g_browser_process->local_state()->GetList(               \
                    growser_ru_trust::kRuTrustDomainsPref)                \
              : nullptr));

#include "brave/components/growser_ru_trust/ru_trust_anchors.h"
#include "brave/components/growser_ru_trust/ru_trust_updater.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"

#include <chrome/browser/net/profile_network_context_service.cc>
#undef BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY
#undef BRAVE_REGISTER_CA_CERTIFICATES_WITH_CONSTRAINTS_PREF
