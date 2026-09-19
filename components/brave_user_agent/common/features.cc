// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_user_agent/common/features.h"

#include "base/feature_list.h"

namespace brave_user_agent {
namespace features {

// Growser-244: off, because it cannot do anything here.
//
// This feature exists to hide the "Brave" brand from sites that break on it:
// a component downloads brave-checks.txt, and the network delegate rewrites
// "Brave" to "Google Chrome" in Sec-CH-UA for a tab on that list. growser#82
// already sends "Google Chrome" to EVERY site
// (chromium_src/components/embedder_support/user_agent_utils.cc), so the
// rewrite searches for a substring that is never present and the exception
// list decides, per request, whether to do that nothing.
//
// What it was not free: with the feature on we register the component at every
// launch and ask Brave's component server for the list - which answers a fork
// 403 (lesson 30). Turning the flag off ends the fetch and leaves every call
// site intact and compiling: ShouldHideBraveBrand() answers false, which is
// the truthful answer when there is no Brave brand to hide.
BASE_FEATURE(kUseBraveUserAgent,
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_IOS)
BASE_FEATURE_PARAM(int,
                   kBraveIOSUserAgentDefault,
                   &kUseBraveUserAgent,
                   "default_user_agent",
                   2);  // BraveIOSUserAgentTypeSuffix
#endif

BASE_FEATURE(kShouldCancelRequestsForUserAgentChange,
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
}  // namespace brave_user_agent
