// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_origin/brave_origin_service_bridge.h"

#include "base/strings/sys_string_conversions.h"
#include "components/policy/policy_constants.h"

// Growser-262: only the two policies our own list still carries. The seven
// that went with their features are named in the header, with why.
BraveOriginPolicyKey const BraveOriginPolicyKeyP3AEnabled =
    base::SysUTF8ToNSString(policy::key::kGrowserP3AEnabled);
BraveOriginPolicyKey const BraveOriginPolicyKeyPlaylistEnabled =
    base::SysUTF8ToNSString(policy::key::kGrowserPlaylistEnabled);
