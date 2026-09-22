/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
#define BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/global_privacy_control/pref_names.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/query_filter/common/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(DEPRECATE_IPFS)
#include "brave/components/ipfs/ipfs_prefs.h"  // nogncheck
#endif                                         // BUILDFLAG(DEPRECATE_IPFS)

#if BUILDFLAG(ENABLE_BRAVE_NEWS)
#include "brave/components/brave_news/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/components/brave_talk/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "brave/components/email_aliases/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_PSST)
#include "brave/components/psst/core/browser/pref_names.h"
#endif

namespace policy {

inline constexpr PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
    // Growser-62: seven policies are absent from this
    // map on purpose - the features they governed are not
    // in this browser, so their .yaml definitions were
    // removed and policy::key:: has no such members. They
    // were guarded by their features' buildflags here,
    // which hid the fact until a build turned one of the
    // features on (growser#265).
    {policy::key::kBraveShieldsDisabledForUrls,
     kManagedBraveShieldsDisabledForUrls, base::Value::Type::LIST},
    {policy::key::kBraveShieldsEnabledForUrls,
     kManagedBraveShieldsEnabledForUrls, base::Value::Type::LIST},
    {policy::key::kGrowserSyncUrl, brave_sync::kCustomSyncServiceUrl,
     base::Value::Type::STRING},
#if BUILDFLAG(ENABLE_TOR)
    {policy::key::kTorDisabled, tor::prefs::kTorDisabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_LOCAL_AI)
    {policy::key::kGrowserLocalAIEnabled, local_ai::prefs::kBraveLocalAIEnabled,
     base::Value::Type::BOOLEAN},
#endif
    {policy::key::kGrowserP3AEnabled, p3a::kP3AEnabled,
     base::Value::Type::BOOLEAN},
#if BUILDFLAG(ENABLE_PLAYLIST)
    {policy::key::kGrowserPlaylistEnabled, playlist::kPlaylistEnabledPref,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_SPEEDREADER)
    {policy::key::kGrowserSpeedreaderEnabled, speedreader::kSpeedreaderEnabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
    {policy::key::kGrowserWaybackMachineEnabled, kBraveWaybackMachineEnabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(DEPRECATE_IPFS)
    {policy::key::kIPFSEnabled, ipfs::prefs::kIPFSEnabledByPolicy,
     base::Value::Type::BOOLEAN},
#endif  // BUILDFLAG(DEPRECATE_IPFS)
#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
    {policy::key::kEmailAliasesEnabled,
     email_aliases::prefs::kEmailAliasesEnabled, base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_PSST)
    {policy::key::kPsstEnabled, psst::prefs::kPsstEnabled,
     base::Value::Type::BOOLEAN},
#endif
    {policy::key::kGrowserReduceLanguageEnabled,
     brave_shields::prefs::kReduceLanguageEnabled, base::Value::Type::BOOLEAN},
    {policy::key::kGrowserDeAmpEnabled, de_amp::kDeAmpPrefEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kGrowserDebouncingEnabled, debounce::prefs::kDebounceEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kGrowserTrackingQueryParametersFilteringEnabled,
     query_filter::kTrackingQueryParametersFilteringEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kGrowserGlobalPrivacyControlEnabled,
     global_privacy_control::kGlobalPrivacyControlEnabled,
     base::Value::Type::BOOLEAN},
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
