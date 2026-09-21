/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/growser_snap/snap.h"

#include "base/scoped_environment_variable_override.h"
#include "content/public/common/content_switches.h"
#include "sandbox/policy/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace growser {

TEST(GrowserSnapTest, NotInSnapWithoutTheVariable) {
  base::ScopedEnvironmentVariableOverride unset("GROWSER_SNAP");
  EXPECT_FALSE(IsRunningInSnap(unset.GetEnv()));
  EXPECT_FALSE(IsSnapPackagingFlag(sandbox::policy::switches::kNoSandbox));
}

TEST(GrowserSnapTest, InSnapWhenTheVariableIsOne) {
  base::ScopedEnvironmentVariableOverride set("GROWSER_SNAP", "1");
  EXPECT_TRUE(IsRunningInSnap(set.GetEnv()));
  EXPECT_TRUE(IsSnapPackagingFlag(sandbox::policy::switches::kNoSandbox));
}

TEST(GrowserSnapTest, OnlyTheExactValueCounts) {
  base::ScopedEnvironmentVariableOverride set("GROWSER_SNAP", "true");
  EXPECT_FALSE(IsRunningInSnap(set.GetEnv()));
  EXPECT_FALSE(IsSnapPackagingFlag(sandbox::policy::switches::kNoSandbox));
}

// The suppression is for the one switch the snap itself passes. A flag a
// person added on top still warns inside the snap.
TEST(GrowserSnapTest, OtherBadFlagsStillWarnInsideTheSnap) {
  base::ScopedEnvironmentVariableOverride set("GROWSER_SNAP", "1");
  EXPECT_FALSE(IsSnapPackagingFlag(switches::kDisableWebSecurity));
  EXPECT_FALSE(
      IsSnapPackagingFlag(sandbox::policy::switches::kDisableGpuSandbox));
  EXPECT_FALSE(IsSnapPackagingFlag(""));
}

}  // namespace growser
