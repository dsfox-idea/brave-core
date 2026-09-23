/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "brave/browser/android/safe_browsing/features.h"
#include "brave/browser/android/youtube_script_injector/features.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_origin/features.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/debounce/core/common/features.h"
#include "brave/components/email_aliases/buildflags/buildflags.h"
#include "brave/components/google_sign_in_permission/features.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/playlist/core/common/buildflags/buildflags.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "brave/components/webcompat/core/common/features.h"
#include "net/base/features.h"
#include "third_party/blink/public/common/features.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/features.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/common/features.h"
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
#include "brave/components/email_aliases/features.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_AI_CHAT_FLAGS                                        \
  &ai_chat::features::kAIChat, &ai_chat::features::kAIChatHistory, \
      &ai_chat::features::kBraveSyncAIChat,
#else
// Growser-270: Leo is out of the product.
// Growser-295: Java still asks about it by name, and FeatureMap CHECK-fails
// on a name it does not hold (Settings crashed that way), so the answer is
// "off".
namespace {
BASE_FEATURE(kGrowserAIChatOff,
             "AIChat",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kGrowserAIChatHistoryOff,
             "AIChatHistory",
             base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kGrowserBraveSyncAIChatOff,
             "BraveSyncAIChat",
             base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_AI_CHAT_FLAGS \
  &kGrowserAIChatOff, &kGrowserAIChatHistoryOff, &kGrowserBraveSyncAIChatOff,
#endif  // BUILDFLAG(ENABLE_AI_CHAT)

#if BUILDFLAG(ENABLE_REQUEST_OTR)
#include "brave/components/request_otr/common/features.h"
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_REQUEST_OTR_FLAG &request_otr::features::kBraveRequestOTRTab,
#else
// Growser: request-OTR is compiled out of this build.
// Growser-295: Java still asks about it by name, and FeatureMap CHECK-fails
// on a name it does not hold (Settings crashed that way), so the answer is
// "off".
namespace {
BASE_FEATURE(kGrowserBraveRequestOTRTabOff,
             "BraveRequestOTRTab",
             base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_REQUEST_OTR_FLAG &kGrowserBraveRequestOTRTabOff,
#endif  // BUILDFLAG(ENABLE_REQUEST_OTR)

#if BUILDFLAG(ENABLE_PLAYLIST)
#include "brave/components/playlist/core/common/features.h"
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_PLAYLIST_FLAG &playlist::features::kPlaylist,
#else
// Growser-273: the playlist is out of the product.
// Growser-295: Java still asks about it by name, and FeatureMap CHECK-fails
// on a name it does not hold (Settings crashed that way), so the answer is
// "off".
namespace {
BASE_FEATURE(kGrowserPlaylistOff,
             "Playlist",
             base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_PLAYLIST_FLAG &kGrowserPlaylistOff,
#endif  // BUILDFLAG(ENABLE_PLAYLIST)

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/features.h"
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_REWARDS_FLAG &brave_rewards::features::kBraveRewards,
#else
// Growser-271: rewards is out of the product.
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_REWARDS_FLAG
#endif  // BUILDFLAG(ENABLE_BRAVE_REWARDS)

#if BUILDFLAG(ENABLE_BRAVE_ADS)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_NEW_TAB_PAGE_AD_FLAG &brave_ads::kNewTabPageAdFeature,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_NEW_TAB_PAGE_AD_FLAG
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "brave/components/web_discovery/common/features.h"
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_WEB_DISCOVERY_FLAG \
  &web_discovery::features::kBraveWebDiscoveryNative,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_WEB_DISCOVERY_FLAG
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_VPN_FLAG &brave_vpn::features::kBraveVPNLinkSubscriptionAndroidUI,
#else
// CHROMIUM_SRC_INTERNAL_USE
#define BRAVE_VPN_FLAG
#endif

#if BUILDFLAG(ENABLE_EMAIL_ALIASES)
// CHROMIUM_SRC_INTERNAL_USE
#define EMAIL_ALIASES_FLAG &email_aliases::features::kEmailAliases,
#else
// Growser-295: Java still asks about it by name, and FeatureMap CHECK-fails
// on a name it does not hold (Settings crashed that way), so the answer is
// "off".
namespace {
BASE_FEATURE(kGrowserEmailAliasesOff,
             "EmailAliases",
             base::FEATURE_DISABLED_BY_DEFAULT);
}  // namespace
// CHROMIUM_SRC_INTERNAL_USE
#define EMAIL_ALIASES_FLAG &kGrowserEmailAliasesOff,
#endif

// clang-format off
#define kForceWebContentsDarkMode kForceWebContentsDarkMode,                   \
    BRAVE_AI_CHAT_FLAGS                                                        \
    BRAVE_NEW_TAB_PAGE_AD_FLAG                                                 \
    BRAVE_WEB_DISCOVERY_FLAG                                                   \
    BRAVE_VPN_FLAG                                                             \
    EMAIL_ALIASES_FLAG                                                         \
    BRAVE_REWARDS_FLAG                                                         \
    &brave_search_conversion::features::kOmniboxBanner,                        \
    BRAVE_PLAYLIST_FLAG                                                        \
    &download::features::kParallelDownloading,                                 \
    &preferences::features::kBraveBackgroundVideoPlayback,                     \
    &preferences::features::kBravePictureInPictureForYouTubeVideos,            \
    BRAVE_REQUEST_OTR_FLAG                                     \
    &safe_browsing::features::kBraveAndroidSafeBrowsing,                       \
    &debounce::features::kBraveDebounce,                                       \
    &webcompat::features::kBraveWebcompatExceptionsService,                    \
    &net::features::kBraveHttpsByDefault,                                      \
    &net::features::kBraveFallbackDoHProvider,                                 \
    &google_sign_in_permission::features::kBraveGoogleSignInPermission,        \
    &net::features::kBraveForgetFirstPartyStorage,                             \
    &brave_shields::features::kBraveShowStrictFingerprintingMode,              \
    &brave_shields::features::kBlockAllCookiesToggle,                          \
    &brave_shields::features::kBraveShieldsElementPicker,                      \
    &features::kBraveAndroidDynamicColorsByDefault,                            \
    &features::kBraveFreshNtpAfterIdleExperiment,                              \
    &ntp_background_images::features::kBraveNTPBrandedWallpaperSurveyPanelist, \
    &brave_shields::features::kBraveShredFeature,                              \
    &brave_origin::features::kBraveOrigin,                                     \
    &features::kBraveCustomSearchEngines,                                      \
    &features::kBraveAndroidTabGroupsSettings

// clang-format on

#include <chrome/browser/flags/android/chrome_feature_list.cc>
#undef kForceWebContentsDarkMode
#undef BRAVE_REQUEST_OTR_FLAG
#undef BRAVE_PLAYLIST_FLAG
#undef BRAVE_REWARDS_FLAG
#undef BRAVE_AI_CHAT_FLAGS
#undef BRAVE_NEW_TAB_PAGE_AD_FLAG
#undef BRAVE_WEB_DISCOVERY_FLAG
#undef BRAVE_VPN_FLAG
#undef EMAIL_ALIASES_FLAG
