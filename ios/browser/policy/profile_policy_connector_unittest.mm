// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/chrome/browser/policy/model/profile_policy_connector.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/growser_policy/buildflags/buildflags.h"
#include "ios/chrome/browser/policy/model/browser_policy_connector_ios.h"
#include "ios/chrome/browser/policy/model/configuration_policy_handler_list_factory.h"
#include "ios/web/public/test/web_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

// growser#386: with external policy compiled out (#358) the browser connector
// has no platform provider, and the profile connector used to hand that null
// to PolicyServiceImpl - a crash on every launch of 26.1022.
using GrowserProfilePolicyConnectorTest = PlatformTest;

#if !BUILDFLAG(GROWSER_ENABLE_EXTERNAL_POLICY)
TEST_F(GrowserProfilePolicyConnectorTest, InitWithoutPlatformProvider) {
  web::WebTaskEnvironment task_environment;
  BrowserPolicyConnectorIOS browser_connector(base::BindRepeating(
      &BuildPolicyHandlerList,
      /*are_future_policies_allowed_by_default=*/true));
  ASSERT_TRUE(browser_connector.GetPolicyService());
  ASSERT_EQ(nullptr, browser_connector.GetPlatformProvider());

  ProfilePolicyConnector profile_connector;
  profile_connector.Init(browser_connector.GetSchemaRegistry(),
                         &browser_connector, /*user_policy_provider=*/nullptr,
                         /*policy_store=*/nullptr);
  EXPECT_TRUE(profile_connector.GetPolicyService());

  profile_connector.GetBraveProfilePolicyProvider()->Shutdown();
  browser_connector.Shutdown();
}
#endif
