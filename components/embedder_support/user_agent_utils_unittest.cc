/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/embedder_support/user_agent_utils.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/test/scoped_command_line.h"
#include "components/embedder_support/switches.h"
#include "components/version_info/version_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"

namespace embedder_support {

namespace {

bool ContainsBrandVersion(const blink::UserAgentBrandList& brand_list,
                          const blink::UserAgentBrandVersion brand_version) {
  for (const auto& brand_list_entry : brand_list) {
    if (brand_list_entry == brand_version)
      return true;
  }
  return false;
}

}  // namespace

// growser (#82): the brand a site reads must be Chrome's, not this browser's.
// Sites that decide we are not Chrome stop loading, so the client-hint brand
// list is one of the places we must be indistinguishable. This is the gate on
// that, and it is deliberately strict about the name: "Brave" is as wrong as
// "Growser" would be.
TEST(UserAgentUtilsTest, UserAgentMetadata) {
  auto metadata = GetUserAgentMetadata();

  const std::string major_version = version_info::GetMajorVersionNumber();
  EXPECT_TRUE(ContainsBrandVersion(metadata.brand_version_list,
                                   {"Google Chrome", major_version}));
  EXPECT_TRUE(ContainsBrandVersion(metadata.brand_version_list,
                                   {"Chromium", major_version}));
  EXPECT_FALSE(ContainsBrandVersion(metadata.brand_version_list,
                                    {"Brave", major_version}));
  EXPECT_FALSE(ContainsBrandVersion(metadata.brand_version_list,
                                    {"Growser", major_version}));
}

TEST(UserAgentUtilsTest, UserAgentFromCommandLine) {
  constexpr char kCmdUserAgentValue[] =
      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36";
  base::test::ScopedCommandLine command_line;
  command_line.GetProcessCommandLine()->AppendSwitchASCII(kUserAgent,
                                                          kCmdUserAgentValue);
  ASSERT_TRUE(
      base::FeatureList::IsEnabled(blink::features::kUACHOverrideBlank));
  const auto brave_metadata = GetUserAgentMetadata();
  const blink::UserAgentMetadata empty_metadata;

  EXPECT_EQ(GetUserAgent(), kCmdUserAgentValue);
  EXPECT_EQ(brave_metadata, empty_metadata);
}

}  // namespace embedder_support
