/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_main_delegate.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/base_switches.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
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
#include "content/public/common/content_switches.h"
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

  // growser (#78): sync is switched off at brave_sync::features::kBraveSync,
  // not here. Appending --disable-sync from this function does nothing:
  // ChromeBrowserMainParts::PreProfileInit runs later and removes the switch
  // whenever that feature is on.

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

#if BUILDFLAG(IS_MAC)
// growser (#206): our user data used to live in Brave's own directory,
// ~/Library/Application Support/BraveSoftware/Brave-Browser. That was never a
// branding blemish. Brave is a real product that reads exactly that path, so
// on a machine with both installed the two browsers shared ONE profile - the
// same cookies, extensions, passwords and crash dumps, belonging to whichever
// ran last. #128 fixed the same thing on Windows, where nothing had to be
// carried over because the development builds already wrote to Growser's own
// path; on macOS every build so far wrote to Brave's, so renaming the
// directory on its own would hand a user an empty browser on their next
// update.
//
// The migration COPIES, and that is the shape of it rather than a precaution:
// the old directory is not ours to take. Nothing in it records who wrote
// what, so "move our data to its new home" is a question with no answer,
// while copying never has to ask it. What stays behind stays Brave's.
namespace growser {
namespace {

// The product component of brave_product_dir_name in brave/build/config.gni,
// and the two halves of what that used to be. scripts/check-user-data-dir.py
// holds the built app against this name, so a later rename in config.gni
// cannot pass a build while this code goes on carrying data from a path
// nothing writes to any more.
constexpr std::string_view kProductDir = "Growser";
constexpr std::string_view kLegacyCompanyDir = "BraveSoftware";
constexpr std::string_view kLegacyProductDir = "Brave-Browser";

// An interrupted copy must not be mistaken for a finished profile, so the
// copy lands here and is renamed into place in a single step.
constexpr std::string_view kStagingSuffix = ".migrating";

}  // namespace

base::FilePath LegacyUserDataDirFor(const base::FilePath& target) {
  const std::string name = target.BaseName().value();
  if (!name.starts_with(kProductDir)) {
    return base::FilePath();
  }
  // Whatever config.gni appended for the channel ("-Development", "-Beta")
  // was appended to the old name too, so the suffix carries straight across.
  const std::string suffix = name.substr(kProductDir.size());
  return target.DirName()
      .Append(kLegacyCompanyDir)
      .Append(std::string(kLegacyProductDir) + suffix);
}

bool MigrateUserDataDir(const base::FilePath& legacy,
                        const base::FilePath& target) {
  if (legacy.empty() || target.empty()) {
    return false;
  }
  // Anything already at the target is a profile in use. Never write over it.
  if (base::PathExists(target)) {
    return false;
  }
  if (!base::DirectoryExists(legacy)) {
    return false;
  }

  // Copy into a staging directory and rename it into place, so a migration
  // killed halfway through leaves no half-profile that the next start would
  // take for a finished one.
  //
  // CopyDirectory takes regular files and directories and skips the rest,
  // deciding on the enumerator's lstat BEFORE it opens anything - so the
  // entries a profile holds while a browser is running on it (SingletonLock,
  // a symlink to a "hostname-pid" string that is not a path, and
  // SingletonSocket) are stepped over rather than failing the copy. They
  // belong to the running browser rather than to the profile and Chromium
  // remakes them, so leaving them out is also the correct copy.
  const base::FilePath staging(target.value() + std::string(kStagingSuffix));
  base::DeletePathRecursively(staging);
  if (!base::CopyDirectory(legacy, staging, /*recursive=*/true) ||
      !base::Move(staging, target)) {
    base::DeletePathRecursively(staging);
    LOG(ERROR) << "growser: could not carry the user data directory over from "
               << legacy << "; starting with an empty one at " << target;
    return false;
  }
  LOG(WARNING) << "growser: carried the user data directory over from "
               << legacy << " to " << target;
  return true;
}

}  // namespace growser
#endif  // BUILDFLAG(IS_MAC)

void BraveMainDelegate::PreSandboxStartup() {
#if BUILDFLAG(IS_MAC)
  // growser (#206): before the base call, because that is what resolves and
  // CREATES the user data directory (ChromeMainDelegate::PreSandboxStartup ->
  // InitializeUserDataDir). Once it exists, the migration's own predicate -
  // that the target is not there yet - is false for good. Browser process
  // only, and never when the user named a directory themselves.
  {
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    if (!command_line.HasSwitch(switches::kProcessType) &&
        !command_line.HasSwitch(switches::kUserDataDir)) {
      base::FilePath target;
      if (chrome::GetDefaultUserDataDirectory(&target)) {
        growser::MigrateUserDataDir(growser::LegacyUserDataDirFor(target),
                                    target);
      }
    }
  }
#endif  // BUILDFLAG(IS_MAC)
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
