/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_component_loader.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/constants/pref_names.h"
// Growser-212
#include "base/files/file_path.h"
#include "brave/components/webharvester/grit/webharvester.h"
#include "brave/components/webharvester/webharvester.h"
#include "brave/components/web_discovery/buildflags/buildflags.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/mojom/manifest.mojom.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
#include "brave/components/web_discovery/common/features.h"
#endif

namespace extensions {

BraveComponentLoader::BraveComponentLoader(Profile* profile)
    : ComponentLoader(profile),
      profile_(profile),
      profile_prefs_(profile->GetPrefs()) {
  pref_change_registrar_.Init(profile_prefs_);
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  pref_change_registrar_.Add(
      kWebDiscoveryEnabled,
      base::BindRepeating(&BraveComponentLoader::UpdateBraveExtension,
                          base::Unretained(this)));
#endif
  // Growser-212: webharvester is off by default and the pref is the switch.
  pref_change_registrar_.Add(
      webharvester::kEnabledPref,
      base::BindRepeating(&BraveComponentLoader::UpdateWebharvesterExtension,
                          base::Unretained(this)));
}

BraveComponentLoader::~BraveComponentLoader() = default;

void BraveComponentLoader::AddDefaultComponentExtensions(
    bool skip_session_components) {
  ComponentLoader::AddDefaultComponentExtensions(skip_session_components);
  UpdateBraveExtension();
  UpdateWebharvesterExtension();
}

bool BraveComponentLoader::UseBraveExtensionBackgroundPage() {
#if BUILDFLAG(ENABLE_WEB_DISCOVERY)
  bool native_enabled = false;
#if BUILDFLAG(ENABLE_WEB_DISCOVERY_NATIVE)
  native_enabled = base::FeatureList::IsEnabled(
      web_discovery::features::kBraveWebDiscoveryNative);
#endif
  return !native_enabled && profile_prefs_->GetBoolean(kWebDiscoveryEnabled);
#else
  return false;
#endif  // BUILDFLAG(ENABLE_WEB_DISCOVERY)
}

void BraveComponentLoader::UpdateBraveExtension() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDisableBraveExtension)) {
    return;
  }

  base::FilePath brave_extension_path(FILE_PATH_LITERAL(""));
  brave_extension_path =
      brave_extension_path.Append(FILE_PATH_LITERAL("brave_extension"));
  auto& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  std::optional<base::DictValue> manifest = base::JSONReader::ReadDict(
      resource_bundle.LoadDataResourceString(IDR_BRAVE_EXTENSION),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  CHECK(manifest) << "invalid Brave Extension manifest";

  // The background page is a conditional. Replace MAYBE_background in the
  // manifest to "background" or remove it.
  auto background_value = manifest->Extract("MAYBE_background");
  if (UseBraveExtensionBackgroundPage() && background_value) {
    manifest->Set("background", std::move(*background_value));
  }

  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile_);
  const Extension* current_extension =
      registry->GetInstalledExtension(brave_extension_id);

  if (current_extension) {
    const auto* current_manifest = current_extension->manifest();
    if (current_manifest && *current_manifest->value() == *manifest) {
      return;  // Skip reload, nothing is actually changed.
    }
    Remove(brave_extension_id);
  }

  const auto id = Add(std::move(*manifest), brave_extension_path);
  CHECK_EQ(id, brave_extension_id);
}

// Growser-212: webharvester travels with the browser but does not run until
// somebody says so. The pref is the durable answer; the switch is for an agent
// that launches the browser itself and wants the grant to end with the session.
bool BraveComponentLoader::WebharvesterEnabled() const {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
             webharvester::kEnableSwitch) ||
         profile_prefs_->GetBoolean(webharvester::kEnabledPref);
}

void BraveComponentLoader::UpdateWebharvesterExtension() {
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile_);
  const bool installed =
      registry->GetInstalledExtension(webharvester::kExtensionId) != nullptr;
  const bool wanted = WebharvesterEnabled();
  if (installed == wanted) {
    return;
  }
  if (!wanted) {
    Remove(webharvester::kExtensionId);
    return;
  }

  std::optional<base::DictValue> manifest = base::JSONReader::ReadDict(
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
          IDR_WEBHARVESTER_MANIFEST),
      base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  CHECK(manifest) << "invalid webharvester manifest";

  // The root must match the resource_path prefix in the extension's grd; the
  // resource manager serves its files by that name.
  const auto id = Add(std::move(*manifest),
                      base::FilePath::FromASCII(webharvester::kExtensionRoot));
  CHECK_EQ(id, webharvester::kExtensionId);
}

}  // namespace extensions
