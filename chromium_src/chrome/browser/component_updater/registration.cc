/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/registration.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"

// growser: drop the Media Engagement Index preload component.
//
// It is a list Google curates of sites allowed to autoplay with sound - an
// editorial policy, not a privacy or security mechanism, and the only one of
// the components that install successfully for us that buys the user nothing.
// The header comes first so the declaration is parsed before the name turns
// into a macro; the call inside registration.cc then compiles away.
#include "chrome/browser/component_updater/mei_preload_component_installer.h"
#define RegisterMediaEngagementPreloadComponent(...) ((void)0)

// growser (#106): drop six more registrations, each serving a feature that
// is compiled out or force-disabled in this fork. They cost every user a
// poll of our backend proxy per update cycle and disk for payloads nothing
// reads: First-Party Sets (RWS disabled by our settings substitution),
// Optimization Hints (remote fetching stubbed to false), OnDeviceHeadSuggest
// (Google-search suggestions; our search is different), CrowdDeny
// (notification crowd data for a Google service), CommerceHeuristics
// (commerce::* features all off), ActorSafetyLists (agent features off).
// Same idiom as MEI above: header first, then the name becomes a macro.
#include "chrome/browser/component_updater/commerce_heuristics_component_installer.h"
#include "chrome/browser/component_updater/crowd_deny_component_installer.h"
#include "chrome/browser/component_updater/first_party_sets_component_installer.h"
#include "components/component_updater/installer_policies/actor_safety_lists_component_installer.h"
#include "components/component_updater/installer_policies/on_device_head_suggest_component_installer.h"
#include "components/component_updater/installer_policies/optimization_hints_component_installer.h"
#define RegisterCommerceHeuristicsComponent(...) ((void)0)
#define RegisterCrowdDenyComponent(...) ((void)0)
#define RegisterFirstPartySetsComponent(...) ((void)0)
#define RegisterActorSafetyListsComponent(...) ((void)0)
#define RegisterOnDeviceHeadSuggestComponent(...) ((void)0)
#define RegisterOptimizationHintsComponent(...) ((void)0)

#define RegisterComponentsForUpdate RegisterComponentsForUpdate_ChromiumImpl

#include <chrome/browser/component_updater/registration.cc>

#undef RegisterComponentsForUpdate
#undef RegisterMediaEngagementPreloadComponent
#undef RegisterCommerceHeuristicsComponent
#undef RegisterCrowdDenyComponent
#undef RegisterFirstPartySetsComponent
#undef RegisterActorSafetyListsComponent
#undef RegisterOnDeviceHeadSuggestComponent
#undef RegisterOptimizationHintsComponent

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_global_features.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/extension_malware_blocklist/browser/extension_malware_blocklist_component_installer.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "brave/components/p3a/buildflags/buildflags.h"
#if BUILDFLAG(ENABLE_P3A)
#include "brave/components/p3a/component_installer.h"
#include "brave/components/p3a/p3a_service.h"
#endif
#include "brave/components/psst/buildflags/buildflags.h"
#include "brave/components/query_filter/browser/query_filter_component_installer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/component_updater_utils.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
#include "chrome/browser/component_updater/translate_kit_component_installer.h"
#include "components/on_device_translation/features.h"
#endif  // BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)

#if BUILDFLAG(ENABLE_PSST)
#include "brave/components/psst/core/browser/psst_component_installer.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/web_mcp/core/browser/web_mcp_component_installer.h"
#include "third_party/blink/public/common/features.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/component_updater/zxcvbn_data_component_installer.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/local_ai/core/local_models_updater.h"
#include "brave/components/local_ai/core/on_device_speech_models_component_installer.h"
#endif

namespace component_updater {

void RegisterComponentsForUpdate() {
  RegisterComponentsForUpdate_ChromiumImpl();
  ComponentUpdateService* cus = g_browser_process->component_updater();
#if BUILDFLAG(ENABLE_BRAVE_WALLET)
  brave_wallet::WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponent(cus,
                                             g_browser_process->local_state());
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)
#if BUILDFLAG(ENABLE_PSST)
  psst::RegisterPsstComponent(cus);
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  // Only fetch WebMCP tool scripts when AI Chat is enabled and the WebMCP
  // feature is enabled. Note: this is the process-wide feature/policy state,
  // not per-profile as components are shared across all profiles.
  if (ai_chat::features::IsAIChatEnabled() &&
      base::FeatureList::IsEnabled(blink::features::kWebMCP)) {
    web_mcp::RegisterWebMcpComponent(cus);
  }
#endif
#if BUILDFLAG(ENABLE_P3A)
  p3a::MaybeToggleP3AComponent(cus, g_brave_browser_process->p3a_service());
#endif
#if BUILDFLAG(IS_ANDROID)
  // Currently behind !BUILDFLAG(IS_ANDROID) in upstream.
  RegisterZxcvbnDataComponent(cus);
#endif  // BUILDFLAG(IS_ANDROID)
  // growser: Brave's user-agent exceptions component is not registered. It
  // lists sites where Brave has to soften its own user agent; ours is already
  // Chrome's, byte for byte, because sites must not be able to tell us apart -
  // so there is nothing for the exceptions to fix.
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_malware_blocklist::RegisterExtensionMalwareBlocklistComponent(
      cus,
      BraveGlobalFeatures::FromGlobalFeatures(g_browser_process->GetFeatures())
          ->extension_malware_blocklist());
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)
#if BUILDFLAG(ENABLE_LOCAL_AI)
  local_ai::ManageLocalModelsComponentRegistration(
      cus, g_browser_process->local_state());
  local_ai::RegisterOnDeviceSpeechModelsComponent(cus);
#endif
  RegisterQueryFilterComponent(cus);

#if BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
  // growser (#70): install the translation ENGINE, not only the language packs.
  //
  // Upstream registers TranslateKit with force_install=false, which does
  // nothing until something has already asked for it once - and the only thing
  // that asks is the Translator API, which refuses to install anything without
  // a user gesture in the page. Our translation starts from browser UI, which
  // gives the page no gesture, so that first ask can never happen: measured as
  // NotAllowedError with the language pack already sitting on disk and
  // availability() answering "downloadable" forever.
  //
  // The packs are pre-installed too (see the features.cc override), and a pack
  // without the engine that reads it is 56 MB of nothing - so the two belong
  // together, gated on the same feature.
  if (base::FeatureList::IsEnabled(
          on_device_translation::kAutoDownloadTranslateLanguagePacks)) {
    RegisterTranslateKitComponent(cus, g_browser_process->local_state(),
                                  /*force_install=*/true,
                                  /*registered_callback=*/base::OnceClosure(),
                                  /*on_ready_callback=*/base::DoNothing());
  }
#endif  // BUILDFLAG(ENABLE_ON_DEVICE_TRANSLATION)
}

}  // namespace component_updater
