/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_IOS_H_
#define BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_IOS_H_

#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "build/build_config.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "brave/components/email_aliases/pref_names.h"
#endif

namespace policy {

// Growser-262: seven entries are gone from this map - wallet, AI chat,
// rewards, talk, news, VPN and the stats ping. Their `policy::key::` constants
// are generated from the list in
// components/policy/resources/templates/policy_definitions/brave_policies.gni,
// and our list carries 22 policies with none of those among them: we removed
// each policy when we removed the feature it configures. So those names did
// not merely become unreachable, they stopped existing, and this header named
// them unconditionally - three objects failed to compile the first time
// anyone built iOS, with `no member named 'kBraveWalletDisabled' in namespace
// 'policy::key'`.
//
// A buildflag guard is NOT the fix, and that is the part worth remembering:
// the desktop twin (browser/policy/brave_simple_policy_map.h) wraps each of
// these in `#if BUILDFLAG(ENABLE_BRAVE_WALLET)` and friends, which compiles
// only because those flags are off in our build. Turn one back on there and
// the desktop breaks exactly as iOS did, because the policy is still absent
// from the list. An entry has to exist if and only if its policy does, and
// ours do not.
//
// If a feature ever returns, its policy returns to brave_policies.gni first;
// the entry here follows the policy, not the buildflag.
inline constexpr PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
    {
        policy::key::kGrowserP3AEnabled,
        p3a::kP3AEnabled,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kGrowserPlaylistEnabled,
        playlist::kPlaylistEnabledPref,
        base::Value::Type::BOOLEAN,
    },
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
    {
        policy::key::kEmailAliasesEnabled,
        email_aliases::prefs::kEmailAliasesEnabled,
        base::Value::Type::BOOLEAN,
    },
#endif
};

}  // namespace policy

#endif  // BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_IOS_H_
