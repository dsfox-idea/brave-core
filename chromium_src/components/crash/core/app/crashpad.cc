/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/crash/core/app/crashpad.h"

#include "brave/components/growser_policy/buildflags/buildflags.h"

// Growser-360: whether crash reports upload is the user's answer - the
// welcome-screen checkbox and the settings switch (#92). Upstream lets a
// MetricsReportingEnabled policy override it in both directions, read straight
// from the registry or a managed plist and so beyond the providers #358
// compiled out. Unless this build lets external policy manage the browser,
// the policy is not asked.
#if BUILDFLAG(GROWSER_ENABLE_EXTERNAL_POLICY)
#define GROWSER_REPORTING_IS_ENFORCED_BY_POLICY(client, enabled) \
  client->ReportingIsEnforcedByPolicy(enabled)
#else
#define GROWSER_REPORTING_IS_ENFORCED_BY_POLICY(client, enabled) false
#endif

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX)
namespace {
// Split into two places to avoid patching:
// components/brave_vpn/app/v2/agent/main.cc
// Need keep it in sync
constexpr char kBraveVpnAgentAppProcessType[] = "brave-vpn-agent-app";

// Split into two places to avoid patching:
// components/brave_vpn/app/v2/helper/main.cc
// Need keep it in sync
constexpr char kBraveVpnHelperAppProcessType[] = "brave-vpn-helper-app";

#if BUILDFLAG(IS_WIN)
// Split into two places to avoid patching:
// NOLINTNEXTLINE
// components\brave_vpn\browser\connection\win\brave_vpn_helper\brave_vpn_helper_crash_reporter_client.cc
// Need keep it in sync
constexpr char kBraveVPNHelperProcessType[] = "brave-vpn-helper";

// Split into two places to avoid patching:
// NOLINTNEXTLINE
// components\brave_vpn\browser\connection\wireguard\win\brave_vpn_wireguard_service\brave_wireguard_service_crash_reporter_client.cc
// Need keep it in sync
constexpr char kBraveWireguardProcessType[] = "brave-vpn-wireguard-service";
#endif  // BUILDFLAG(IS_WIN)
}  // namespace

#if BUILDFLAG(IS_WIN)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_CRASHPAD_VPN_V1_PROCESS_TYPES     \
  process_type == kBraveVPNHelperProcessType || \
      process_type == kBraveWireguardProcessType ||
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_CRASHPAD_VPN_V1_PROCESS_TYPES
#endif  // BUILDFLAG(IS_WIN)

#define BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE    \
  process_type == kBraveVpnAgentAppProcessType ||      \
      process_type == kBraveVpnHelperAppProcessType || \
      BRAVE_CRASHPAD_VPN_V1_PROCESS_TYPES

#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX)

#include <components/crash/core/app/crashpad.cc>
#undef GROWSER_REPORTING_IS_ENFORCED_BY_POLICY
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_APPLE) || BUILDFLAG(IS_LINUX)
#undef BRAVE_CRASHPAD_VPN_V1_PROCESS_TYPES
#undef BRAVE_INITIALIZE_CRASHPAD_IMPL_PROCESS_TYPE
#endif
