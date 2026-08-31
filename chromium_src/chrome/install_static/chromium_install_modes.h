/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Brand-specific types and constants for Google Chrome.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_

#include <stdlib.h>

#include <array>

#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/common/chrome_icon_resources_win.h"
#include "chrome/install_static/install_constants.h"

namespace install_static {

// Brand-specific constants and install modes for Brave.

// The brand-specific company name to be included as a component of the install
// and user data directory paths. May be empty if no such dir is to be used.
//
// growser: empty, the way unbranded Chromium leaves it. Keeping
// "BraveSoftware" would install us under Brave's directory and, worse, put our
// user data next to theirs; adding a "Growser" company on top of a "Growser"
// product would only produce Growser\Growser\Application.
inline constexpr wchar_t kCompanyPathName[] = L"";

// The brand-specific product name to be included as a component of the install
// and user data directory paths.
#if defined(OFFICIAL_BUILD)
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Brave Origin uses "Brave-Origin" instead of "Brave-Browser" to allow
// side-by-side installation with Brave Browser.
inline constexpr wchar_t kProductPathName[] = L"Brave-Origin";
#else
// growser (#128): the SAME name as the developer mode below, on purpose -
// every existing install and profile lives under "Growser", and the first
// official build must keep those paths, not move them.
inline constexpr wchar_t kProductPathName[] = L"Growser";
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#else
// If you change this, then you also need to change occurrences of this string
// in mini_installer_constants.cc.
//
// growser: the developer mode, used by non-official builds. Since #128 the
// OFFICIAL_BUILD modes above carry the same Growser identity (stable is
// byte-for-byte this mode's identity; beta/dev/nightly are gn-flag debug
// channels with their own fresh GUIDs), so either build kind is the same
// product on disk and in the registry.
//
// The name is "Growser", not "Growser-Development", even though this is
// upstream's developer mode: it decides the install directory, the profile path
// under %LOCALAPPDATA% and what a user sees in Programs and Features. Shipping
// the word "Development" to users would be a slip of the build system into the
// product, and renaming it after anyone has a profile means moving their
// profile.
inline constexpr wchar_t kProductPathName[] = L"Growser";
#endif

// The brand-specific safe browsing client name.
inline constexpr char kSafeBrowsingName[] = "chromium";

// Note: This list of indices must be kept in sync with the brand-specific
// resource strings in chrome/installer/util/prebuild/create_string_rc.
enum InstallConstantIndex {
#if defined(OFFICIAL_BUILD)
  STABLE_INDEX,
  BETA_INDEX,
  DEV_INDEX,
  NIGHTLY_INDEX,
#else
  DEVELOPER_INDEX,
#endif
  NUM_INSTALL_MODES,
};

#if defined(OFFICIAL_BUILD)

// This is overriding the upstream value and shouldn't be undef'ed
// CHROMIUM_SRC_NOLINT
#define CHROMIUM_INDEX STABLE_INDEX

// Regarding the install switch, use the same values that are in
// chrome/installer/mini_installer/configuration.cc
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Brave Origin uses separate identifiers from Brave Browser to allow
// side-by-side installation and independent update infrastructure.
inline constexpr auto kInstallModes = std::to_array<InstallConstants>({
    // The primary install mode for stable Brave Origin.
    {
        .size = sizeof(InstallConstants),
        .index = STABLE_INDEX,  // The first mode is for stable/beta/dev.
        .install_switch =
            "",  // No install switch for the primary install mode.
        .install_suffix =
            L"",  // Empty install suffix - "Origin" is in kProductPathName.
        .logo_suffix = L"",  // No logo suffix for the primary install mode.
        .app_guid = L"{F1EF32DE-F987-4289-81D2-6C4780027F9B}",
        .base_app_name = L"Brave Origin",         // A distinct base_app_name.
        .base_app_id = L"BraveOrigin",            // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveOHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Origin HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "brave-origin",
        .pdf_prog_id_prefix = L"BraveOPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Origin PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{F1EF32DE-F987-4289-81D2-6C4780027F9B}",  // Active Setup GUID.
        .toast_activator_clsid = {0x8a7b6c5d,
                                  0x4e3f,
                                  0x2a1b,
                                  {0x9c, 0x8d, 0x7e, 0x6f, 0x5a, 0x4b, 0x3c,
                                   0x2d}},  // Toast activator CLSID.
        .elevator_clsid = {0x1a2b3c4d,
                           0x5e6f,
                           0x7a8b,
                           {0x9c, 0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c,
                            0x6d}},  // Elevator CLSID.
        .elevator_iid = {0x2b3c4d5e,
                         0x6f7a,
                         0x8b9c,
                         {0x0d, 0x1e, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e}},
        .default_channel_name = L"",  // The empty string means "stable".
        .channel_strategy = ChannelStrategy::FLOATING,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_MAINFRAME,  // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012153-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Origin Beta
    {
        .size = sizeof(InstallConstants),
        .index = BETA_INDEX,  // The mode for the side-by-side beta channel.
        .install_switch = "chrome-beta",  // Install switch.
        .install_suffix = L"-Beta",       // Install suffix.
        .logo_suffix = L"Beta",           // Logo suffix.
        .app_guid =
            L"{56DA94FD-D872-416B-BFC4-1D7011DA7473}",  // A distinct app GUID.
        .base_app_name = L"Brave Origin Beta",     // A distinct base_app_name.
        .base_app_id = L"BraveOriginBeta",         // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveOBHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Origin Beta HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "brave-origin-beta",
        .pdf_prog_id_prefix = L"BraveOBPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Origin Beta PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{56DA94FD-D872-416B-BFC4-1D7011DA7473}",  // Active Setup GUID.
        .toast_activator_clsid = {0x3c4d5e6f,
                                  0x7a8b,
                                  0x9c0d,
                                  {0x1e, 0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e,
                                   0x8f}},  // Toast activator CLSID.
        .elevator_clsid = {0x4d5e6f7a,
                           0x8b9c,
                           0x0d1e,
                           {0x2f, 0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f,
                            0x9a}},  // Elevator CLSID.
        .elevator_iid = {0x5e6f7a8b,
                         0x9c0d,
                         0x1e2f,
                         {0x3a, 0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x9a, 0x0b}},
        .default_channel_name = L"beta",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kBetaApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_X005_BETA,      // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012154-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Origin Dev
    {
        .size = sizeof(InstallConstants),
        .index = DEV_INDEX,  // The mode for the side-by-side dev channel.
        .install_switch = "chrome-dev",  // Install switch.
        .install_suffix = L"-Dev",       // Install suffix.
        .logo_suffix = L"Dev",           // Logo suffix.
        .app_guid =
            L"{716D6A4A-D071-47A8-AC64-DBDE3EE3797B}",  // A distinct app GUID.
        .base_app_name = L"Brave Origin Dev",      // A distinct base_app_name.
        .base_app_id = L"BraveOriginDev",          // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveODHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Origin Dev HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "brave-origin-dev",
        .pdf_prog_id_prefix = L"BraveODPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Origin Dev PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{716D6A4A-D071-47A8-AC64-DBDE3EE3797B}",  // Active Setup GUID.
        .toast_activator_clsid = {0x6f7a8b9c,
                                  0x0d1e,
                                  0x2f3a,
                                  {0x4b, 0x5c, 0x6d, 0x7e, 0x8f, 0x9a, 0x0b,
                                   0x1c}},  // Toast activator CLSID.
        .elevator_clsid = {0x7a8b9c0d,
                           0x1e2f,
                           0x3a4b,
                           {0x5c, 0x6d, 0x7e, 0x8f, 0x9a, 0x0b, 0x1c,
                            0x2d}},  // Elevator CLSID.
        .elevator_iid = {0x8b9c0d1e,
                         0x2f3a,
                         0x4b5c,
                         {0x6d, 0x7e, 0x8f, 0x9a, 0x0b, 0x1c, 0x2d, 0x3e}},
        .default_channel_name = L"dev",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kDevApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_X004_DEV,      // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012155-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Brave Origin SxS (nightly).
    {
        .size = sizeof(InstallConstants),
        .index =
            NIGHTLY_INDEX,  // The mode for the side-by-side nightly channel.
        .install_switch = "chrome-sxs",  // Install switch.
        .install_suffix = L"-Nightly",   // Install suffix.
        .logo_suffix = L"Canary",        // Logo suffix.
        .app_guid =
            L"{50474E96-9CD2-4BC8-B0A7-0D4B6EF2E709}",  // A distinct app GUID.
        .base_app_name = L"Brave Origin Nightly",  // A distinct base_app_name.
        .base_app_id = L"BraveOriginNightly",      // A distinct base_app_id.
        .browser_prog_id_prefix = L"BraveOSHTM",   // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Brave Origin Nightly HTML Document",  // Browser ProgID
                                                    // description.
        .direct_launch_url_scheme = "brave-origin-nightly",
        .pdf_prog_id_prefix = L"BraveOSPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Brave Origin Nightly PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{50474E96-9CD2-4BC8-B0A7-0D4B6EF2E709}",  // Active Setup GUID.
        .toast_activator_clsid = {0x9c0d1e2f,
                                  0x3a4b,
                                  0x5c6d,
                                  {0x7e, 0x8f, 0x9a, 0x0b, 0x1c, 0x2d, 0x3e,
                                   0x4f}},  // Toast activator CLSID.
        .elevator_clsid = {0x0d1e2f3a,
                           0x4b5c,
                           0x6d7e,
                           {0x8f, 0x9a, 0x0b, 0x1c, 0x2d, 0x3e, 0x4f,
                            0x5a}},  // Elevator CLSID.
        .elevator_iid = {0x1e2f3a4b,
                         0x5c6d,
                         0x7e8f,
                         {0x9a, 0x0b, 0x1c, 0x2d, 0x3e, 0x4f, 0x5a, 0x6b}},
        .default_channel_name = L"nightly",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Support system-level installs.
        .supports_set_as_default_browser =
            true,  // Support in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kSxSApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_SXS,           // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012156-",  // App container sid prefix for sandbox.
    },
});
#else   // !BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// growser (#128): every identifier in all four modes is ours. The STABLE
// mode carries the exact identity of the developer mode below - app GUID,
// COM CLSIDs, ProgIDs, sandbox SID - because that is what every existing
// install has registered, and the first official build must be the same
// product, not a new one. Beta/Dev/Nightly exist for gn-flag channel
// debugging (brave_channel=...) with freshly generated identities so a
// debug channel installs side by side and never fights the real one.
inline constexpr auto kInstallModes = std::to_array<InstallConstants>({
    // The primary install mode for stable Growser.
    {
        .size = sizeof(InstallConstants),
        .index = STABLE_INDEX,  // The first mode is for stable/beta/dev.
        .install_switch =
            "",  // No install switch for the primary install mode.
        .install_suffix =
            L"",  // Empty install_suffix for the primary install mode.
        .logo_suffix = L"",  // No logo suffix for the primary install mode.
        .app_guid = L"{B003E671-954C-4C60-A0D4-4172D74FD4C1}",
        .base_app_name = L"Growser",               // A distinct base_app_name.
        .base_app_id = L"Growser",                 // A distinct base_app_id.
        .browser_prog_id_prefix = L"GrowserHTML",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Growser HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "growser",
        .pdf_prog_id_prefix = L"GrowserPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Growser PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{AD0A8A35-2AF7-48DC-A0F0-D2B8CABD7EE9}",  // Active Setup GUID.
        .toast_activator_clsid = {0x83127675,
                                  0xce10,
                                  0x4ac5,
                                  {0x9c, 0x10, 0x3f, 0xf2, 0xe7, 0xd4, 0x53,
                                   0x99}},  // Toast activator CLSID.
        .elevator_clsid = {0xfd440037,
                           0x1c1c,
                           0x4ec3,
                           {0xb4, 0x7a, 0x34, 0xf1, 0xb3, 0x58, 0x8c,
                            0xb6}},  // Elevator CLSID.
        .elevator_iid = {0x87070ee1,
                         0xc7d8,
                         0x473f,
                         {0xb2, 0x34, 0x94, 0x77, 0x97, 0xa8, 0x77, 0xec}},
        .default_channel_name = L"",  // The empty string means "stable".
        // FLOATING is the only primary-mode option here: official builds
        // force USE_GOOGLE_UPDATE_INTEGRATION=1 (install_constants.h
        // override), which compiles UNSUPPORTED out of the enum. Nothing
        // writes an "ap" value under Software\Growser, so the channel
        // floats on an absent value - i.e. stable, same as today.
        .channel_strategy = ChannelStrategy::FLOATING,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_MAINFRAME,  // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012160-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Growser Beta (gn-flag debug channel).
    {
        .size = sizeof(InstallConstants),
        .index = BETA_INDEX,  // The mode for the side-by-side beta channel.
        .install_switch = "chrome-beta",  // Install switch.
        .install_suffix = L"-Beta",       // Install suffix.
        .logo_suffix = L"Beta",           // Logo suffix.
        .app_guid =
            L"{3C158BB8-E5D2-4180-B58E-E3F9BB45F4F6}",  // A distinct app GUID.
        .base_app_name = L"Growser Beta",           // A distinct base_app_name.
        .base_app_id = L"GrowserBeta",              // A distinct base_app_id.
        .browser_prog_id_prefix = L"GrowserBHTM",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Growser Beta HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "growser-beta",
        .pdf_prog_id_prefix = L"GrowserBPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Growser Beta PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{3C158BB8-E5D2-4180-B58E-E3F9BB45F4F6}",  // Active Setup GUID.
        .toast_activator_clsid = {0xdda9dde5,
                                  0x20e3,
                                  0x45a7,
                                  {0xb0, 0x77, 0x05, 0xa5, 0xe9, 0xa2, 0x65,
                                   0x3e}},  // Toast activator CLSID.
        .elevator_clsid = {0xda5873a9,
                           0xc3e0,
                           0x416e,
                           {0x93, 0xc0, 0xdf, 0xe3, 0x2c, 0x20, 0x0f,
                            0x6e}},  // Elevator CLSID.
        .elevator_iid = {0x4a786777,
                         0xf291,
                         0x4e37,
                         {0x89, 0x62, 0x50, 0xd0, 0xed, 0x5f, 0xc2, 0xaf}},
        .default_channel_name = L"beta",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kBetaApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_X005_BETA,      // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012161-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Growser Dev (gn-flag debug channel).
    {
        .size = sizeof(InstallConstants),
        .index = DEV_INDEX,  // The mode for the side-by-side dev channel.
        .install_switch = "chrome-dev",  // Install switch.
        .install_suffix = L"-Dev",       // Install suffix.
        .logo_suffix = L"Dev",           // Logo suffix.
        .app_guid =
            L"{81B04A9D-2573-4FC0-BE7F-79583A591680}",  // A distinct app GUID.
        .base_app_name = L"Growser Dev",            // A distinct base_app_name.
        .base_app_id = L"GrowserDev",               // A distinct base_app_id.
        .browser_prog_id_prefix = L"GrowserDHTM",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Growser Dev HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "growser-dev",
        .pdf_prog_id_prefix = L"GrowserDPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Growser Dev PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{81B04A9D-2573-4FC0-BE7F-79583A591680}",  // Active Setup GUID.
        .toast_activator_clsid = {0x08a67cbc,
                                  0x18f1,
                                  0x4c37,
                                  {0x8c, 0x37, 0x92, 0x60, 0xcd, 0xb5, 0xaa,
                                   0x82}},  // Toast activator CLSID.
        .elevator_clsid = {0x934dfc41,
                           0x966d,
                           0x45af,
                           {0xb4, 0x65, 0xa4, 0x64, 0x71, 0xb3, 0xce,
                            0x9a}},  // Elevator CLSID.
        .elevator_iid = {0x07e21793,
                         0xc8f1,
                         0x4863,
                         {0xbe, 0x43, 0xd7, 0x52, 0x27, 0xb1, 0xb5, 0x0c}},
        .default_channel_name = L"dev",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kDevApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_X004_DEV,      // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012162-",  // App container sid prefix for sandbox.
    },
    // A secondary install mode for Growser Nightly (gn-flag debug channel).
    {
        .size = sizeof(InstallConstants),
        .index =
            NIGHTLY_INDEX,  // The mode for the side-by-side nightly channel.
        .install_switch = "chrome-sxs",  // Install switch.
        .install_suffix = L"-Nightly",   // Install suffix.
        .logo_suffix = L"Canary",        // Logo suffix.
        .app_guid =
            L"{97E119AD-4AD1-473F-AABA-5204810AA9DE}",  // A distinct app GUID.
        .base_app_name = L"Growser Nightly",        // A distinct base_app_name.
        .base_app_id = L"GrowserNightly",           // A distinct base_app_id.
        .browser_prog_id_prefix = L"GrowserNHTM",  // Browser ProgID prefix.
        .browser_prog_id_description =
            L"Growser Nightly HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "growser-nightly",
        .pdf_prog_id_prefix = L"GrowserNPDF",  // PDF ProgID prefix.
        .pdf_prog_id_description =
            L"Growser Nightly PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{97E119AD-4AD1-473F-AABA-5204810AA9DE}",  // Active Setup GUID.
        .toast_activator_clsid = {0x2a6a7aa0,
                                  0xec67,
                                  0x4393,
                                  {0xa2, 0x09, 0x8c, 0x08, 0x87, 0x37, 0x08,
                                   0x96}},  // Toast activator CLSID.
        .elevator_clsid = {0x7b9d07aa,
                           0x6e5d,
                           0x42b8,
                           {0x93, 0x32, 0x0b, 0x2f, 0xa6, 0x91, 0x51,
                            0xe1}},  // Elevator CLSID.
        .elevator_iid = {0x797f1ca7,
                         0x8c30,
                         0x48c3,
                         {0x80, 0xa3, 0x25, 0xc6, 0x62, 0xe3, 0xc9, 0x9f}},
        .default_channel_name = L"nightly",  // Forced channel name.
        .channel_strategy = ChannelStrategy::FIXED,
        .supports_system_level = true,  // Support system-level installs.
        .supports_set_as_default_browser =
            true,  // Support in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kSxSApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_SXS,           // App icon resource id.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012163-",  // App container sid prefix for sandbox.
    },
});
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#else

// CHROMIUM_SRC_NOLINT
#define CHROMIUM_INDEX DEVELOPER_INDEX

inline constexpr auto kInstallModes = std::to_array<InstallConstants>({
    // The primary (and only) install mode for Brave developer build.
    {
        .size = sizeof(InstallConstants),
        .index = DEVELOPER_INDEX,  // The one and only mode for developer mode.
        .install_switch =
            "",  // No install switch for the primary install mode.
        .install_suffix =
            L"",  // Empty install_suffix for the primary install mode.
        .logo_suffix = L"",  // No logo suffix for the primary install mode.
        // growser (#51): we DO have update integration in this build. Brave
        // left this empty because for them an unofficial build never updates -
        // the same assumption that made uninstall crash in #50. Omaha 4
        // registers the browser under exactly this value
        // (chrome/browser/updater/browser_updater_client_win.cc:22), so an
        // empty one means nothing can register.
        .app_guid = L"{B003E671-954C-4C60-A0D4-4172D74FD4C1}",
        // growser: every identifier below is ours. They are not cosmetic - the
        // GUIDs register COM classes machine-wide (the toast activator is what
        // Windows calls back for notifications, the elevator runs the elevated
        // service), so shipping Brave's values would mean two products fighting
        // over the same registrations on any machine that has both. The three
        // CLSIDs/IIDs and the Active Setup GUID are freshly generated for
        // growser and appear nowhere else.
        .base_app_name = L"Growser",                 // A distinct base_app_name.
        .base_app_id = L"Growser",                   // A distinct base_app_id.
        .browser_prog_id_prefix = L"GrowserHTML",  // Browser ProgID prefix (<=11).
        .browser_prog_id_description =
            L"Growser HTML Document",  // Browser ProgID description.
        .direct_launch_url_scheme = "growser",
        .pdf_prog_id_prefix = L"GrowserPDF",  // PDF ProgID prefix (<=11).
        .pdf_prog_id_description =
            L"Growser PDF Document",  // PDF ProgID description.
        .active_setup_guid =
            L"{AD0A8A35-2AF7-48DC-A0F0-D2B8CABD7EE9}",  // Active Setup GUID.
        .toast_activator_clsid = {0x83127675,
                                  0xce10,
                                  0x4ac5,
                                  {0x9c, 0x10, 0x3f, 0xf2, 0xe7, 0xd4, 0x53,
                                   0x99}},  // Toast activator CLSID.
        .elevator_clsid = {0xfd440037,
                           0x1c1c,
                           0x4ec3,
                           {0xb4, 0x7a, 0x34, 0xf1, 0xb3, 0x58, 0x8c,
                            0xb6}},  // Elevator CLSID.
        .elevator_iid = {0x87070ee1,
                         0xc7d8,
                         0x473f,
                         {0xb2, 0x34, 0x94, 0x77, 0x97, 0xa8, 0x77, 0xec}},
        .default_channel_name =
            L"",  // Empty default channel name since no update integration.
        .channel_strategy = ChannelStrategy::UNSUPPORTED,
        .supports_system_level = true,  // Supports system-level installs.
        .supports_set_as_default_browser =
            true,  // Supports in-product set as default browser UX.
        .app_icon_resource_index =
            icon_resources::kApplicationIndex,  // App icon resource index.
        .app_icon_resource_id = IDR_MAINFRAME,  // App icon resource id.
        // growser: distinct trailing RID. This prefix names the sandbox's
        // AppContainer profiles, and Brave gives each channel its own
        // (...148 through ...156) precisely so they do not share; ...148 is
        // Brave Development's, so we take an unused one rather than sit in it.
        .sandbox_sid_prefix =
            L"S-1-15-2-3251537155-1984446955-2931258699-841473695-1938553385-"
            L"934012160-",  // App container sid prefix for sandbox.
    },
});
#endif

}  // namespace install_static

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALL_STATIC_CHROMIUM_INSTALL_MODES_H_
