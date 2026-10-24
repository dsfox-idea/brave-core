// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/policy/core/common/configuration_policy_provider.h"

namespace brave_policy {
std::unique_ptr<policy::ConfigurationPolicyProvider>
CreateBraveProfilePolicyProvider();
}  // namespace brave_policy

// Create and Init the Brave Profile Policy Provider which is used
// for Brave Origin and other Brave specific policies.
// Growser-386: with external policy compiled out (#358) there is no platform
// provider, and upstream pushes the null GetPlatformProvider() into the list
// unchecked; PolicyServiceImpl then dereferences it on every launch.
#define BRAVE_PROFILE_POLICY_CONNECTOR_INIT                          \
  std::erase_if(policy_providers_,                                   \
                [](const auto& provider) { return !provider; });     \
  brave_profile_policy_provider_ =                                   \
      brave_policy::CreateBraveProfilePolicyProvider();              \
  policy_providers_.push_back(brave_profile_policy_provider_.get()); \
  brave_profile_policy_provider_->Init(schema_registry);

#include <ios/chrome/browser/policy/model/profile_policy_connector.mm>

raw_ptr<policy::ConfigurationPolicyProvider>
ProfilePolicyConnector::GetBraveProfilePolicyProvider() {
  return brave_profile_policy_provider_.get();
}

#undef BRAVE_PROFILE_POLICY_CONNECTOR_INIT
