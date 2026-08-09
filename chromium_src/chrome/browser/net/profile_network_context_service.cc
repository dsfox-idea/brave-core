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

#include <chrome/browser/net/profile_network_context_service.cc>
#undef BRAVE_PROFILE_NETWORK_CONTEXT_SERVICE_GET_CT_POLICY
