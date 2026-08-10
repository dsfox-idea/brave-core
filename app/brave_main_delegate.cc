/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <memory>
#include <optional>
#include <string>

#include "base/base_switches.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/common/resource_bundle_helper.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "brave/components/brave_sync/buildflags.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/update_client/buildflags.h"
#include "brave/components/variations/command_line_utils.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "brave/utility/brave_content_utility_client.h"
#include "build/build_config.h"
#include "chrome/app/chrome_main_delegate.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/chrome_switches.h"
#include "components/component_updater/component_updater_switches.h"
#include "components/dom_distiller/core/dom_distiller_switches.h"
#include "components/embedder_support/switches.h"
#include "components/sync/base/command_line_switches.h"
#include "google_apis/gaia/gaia_switches.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"

#if BUILDFLAG(IS_LINUX)
#include "base/linux_util.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "components/signin/public/base/account_consistency_method.h"
#endif
namespace {

constexpr char kBraveOriginTrialsPublicKey[] =
    "bYUKPJoPnCxeNvu72j4EmPuK7tr1PAC7SHh8ld9Mw3E=,"
    "fMS4mpO6buLQ/QMd+zJmxzty/VQ6B1EUZqoCU04zoRU=";

constexpr char kDummyUrl[] = "https://no-thanks.invalid";

std::string GetUpdateURLHost() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(brave_component_updater::kUseGoUpdateDev) &&
      !base::FeatureList::IsEnabled(
          brave_component_updater::kUseDevUpdaterUrl)) {
    return BUILDFLAG(UPDATER_PROD_ENDPOINT);
  }
  return BUILDFLAG(UPDATER_DEV_ENDPOINT);
}

}  // namespace

#if !defined(CHROME_MULTIPLE_DLL_BROWSER)
base::LazyInstance<BraveContentRendererClient>::DestructorAtExit
    g_brave_content_renderer_client = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<BraveContentUtilityClient>::DestructorAtExit
    g_brave_content_utility_client = LAZY_INSTANCE_INITIALIZER;
#endif
#if !defined(CHROME_MULTIPLE_DLL_CHILD)
base::LazyInstance<BraveContentBrowserClient>::DestructorAtExit
    g_brave_content_browser_client = LAZY_INSTANCE_INITIALIZER;
#endif

#if BUILDFLAG(IS_ANDROID)
BraveMainDelegate::BraveMainDelegate() : ChromeMainDelegate() {}
#endif

BraveMainDelegate::BraveMainDelegate(const StartupTimestamps& timestamps)
    : ChromeMainDelegate(timestamps) {}

BraveMainDelegate::~BraveMainDelegate() {}

content::ContentBrowserClient* BraveMainDelegate::CreateContentBrowserClient() {
#if defined(CHROME_MULTIPLE_DLL_CHILD)
  return NULL;
#else
  if (chrome_content_browser_client_ == nullptr) {
    chrome_content_browser_client_ =
        std::make_unique<BraveContentBrowserClient>();
  }
  return chrome_content_browser_client_.get();
#endif
}

content::ContentRendererClient*
BraveMainDelegate::CreateContentRendererClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_renderer_client.Pointer();
#endif
}

content::ContentUtilityClient* BraveMainDelegate::CreateContentUtilityClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return g_brave_content_utility_client.Pointer();
#endif
}

// static
void BraveMainDelegate::AppendCommandLineOptions() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitch(switches::kDisableDomainReliability);
  command_line->AppendSwitch(switches::kEnableDomDistiller);
  command_line->AppendSwitch(switches::kEnableDistillabilityService);

  // growser (#79): sync is off until we run a sync server of our own.
  //
  // The endpoint compiled in is Brave's (BRAVE_SYNC_ENDPOINT ->
  // sync-v2.brave.com/v2) and it refuses a fork: POST /v2/command/ answers
  // 403 "Missing auth header" with no key, with an empty key - which is what
  // we send - and with a wrong one. A sync chain set up here cannot work.
  //
  // The reason to close it rather than leave it visible is that the failure is
  // silent. Every other dead Brave service costs the user nothing when it
  // fails; this is the one where someone puts the bookmarks and passwords they
  // expect to find on their other machine. A switch that quietly does nothing
  // is worse than no switch.
  //
  // Done with Chromium's own switch rather than by overriding
  // IsSyncAllowedByFlag(), because everything already asks that function - the
  // settings section through "isSyncDisabled" (brave_settings_ui.cc), which
  // also decides whether the /braveSync route is created at all, and the menu
  // command in brave_browser_command_controller.cc - and because a hardcoded
  // false leaves the sync unit tests no way to build a sync service (31 of
  // them crash). This path is browser startup only, so the tests are untouched
  // and keep testing what they always did.
  //
  // Reversing this is one line, and reversing it is the plan: when
  // brave_sync_endpoint points at a server of ours, this goes away.
  command_line->AppendSwitch(syncer::kDisableSync);

  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          embedder_support::kOriginTrialPublicKey)) {
    command_line->AppendSwitchASCII(embedder_support::kOriginTrialPublicKey,
                                    kBraveOriginTrialsPublicKey);
  }

  command_line->AppendSwitchASCII(switches::kLsoUrl, kDummyUrl);

  variations::AppendBraveCommandLineOptions(*command_line);
}

std::optional<int> BraveMainDelegate::BasicStartupComplete() {
  BraveMainDelegate::AppendCommandLineOptions();
  return ChromeMainDelegate::BasicStartupComplete();
}

void BraveMainDelegate::PreSandboxStartup() {
  ChromeMainDelegate::PreSandboxStartup();
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
  // Setup NativeMessagingHosts to point to the default Chrome locations
  // because that's where native apps will create them
  base::FilePath chrome_user_data_dir;
  base::FilePath native_messaging_dir;
#if BUILDFLAG(IS_MAC)
  base::PathService::Get(base::DIR_APP_DATA, &chrome_user_data_dir);
  chrome_user_data_dir = chrome_user_data_dir.Append("Google/Chrome");
  native_messaging_dir = base::FilePath(
      FILE_PATH_LITERAL("/Library/Google/Chrome/NativeMessagingHosts"));
#else
  chrome::GetDefaultUserDataDirectory(&chrome_user_data_dir);
  native_messaging_dir = base::FilePath(
      FILE_PATH_LITERAL("/etc/opt/chrome/native-messaging-hosts"));
#endif  // BUILDFLAG(IS_MAC)
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_USER_NATIVE_MESSAGING,
      chrome_user_data_dir.Append(FILE_PATH_LITERAL("NativeMessagingHosts")),
      false, true);
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_NATIVE_MESSAGING, native_messaging_dir, false, true);
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_POSIX) && !BUILDFLAG(IS_MAC)
  base::PathService::OverrideAndCreateIfNeeded(
      chrome::DIR_POLICY_FILES,
      base::FilePath(FILE_PATH_LITERAL("/etc/brave/policies")), true, false);
#endif

#if BUILDFLAG(IS_LINUX)
  // Ensure we have read the Linux distro before the process is sandboxed.
  // Required for choosing the appropriate anti-fingerprinting font allowlist.
  base::GetLinuxDistro();
#endif

  if (brave::SubprocessNeedsResourceBundle()) {
    brave::InitializeResourceBundle();
  }
}

std::optional<int> BraveMainDelegate::PostEarlyInitialization(
    ChromeMainDelegate::InvokedIn invoked_in) {
  auto result = ChromeMainDelegate::PostEarlyInitialization(invoked_in);
  if (result.has_value()) {
    // An exit code is set. Stop initialization.
    return result;
  }

  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string update_url = GetUpdateURLHost();
  if (!update_url.empty()) {
    std::string current_value;
    if (command_line->HasSwitch(switches::kComponentUpdater)) {
      current_value =
          command_line->GetSwitchValueASCII(switches::kComponentUpdater);
      command_line->RemoveSwitch(switches::kComponentUpdater);
    }
    if (!current_value.empty()) {
      current_value += ',';
    }

    command_line->AppendSwitchASCII(
        switches::kComponentUpdater,
        base::StrCat({current_value, "url-source=", update_url}));
  }

  // For Self-host sync service URL
  if (command_line->HasSwitch(syncer::kSyncServiceURL)) {
    GURL sync_service_url =
        GURL(command_line->GetSwitchValueASCII(syncer::kSyncServiceURL));
    // We validate the URL to ensure it meets security requirements:
    // 1. The URL must be valid
    // 2. The URL must use HTTPS (or be otherwise potentially trustworthy like
    // localhost) If the URL doesn't meet these requirements, we remove the
    // switch and use the default sync URL
    if (!sync_service_url.is_valid() ||
        !sync_service_url.SchemeIsHTTPOrHTTPS() ||
        !network::IsOriginPotentiallyTrustworthy(
            url::Origin::Create(sync_service_url))) {
      command_line->RemoveSwitch(syncer::kSyncServiceURL);
      LOG(WARNING) << "Provided sync service URL is invalid or insecure; "
                      "falling back to the default Brave-hosted Sync server.";
    }
  }

  return result;
}
