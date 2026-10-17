/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser (#360): whether crash reports upload is the user's answer. A policy
// the operating system hands the crash client - a registry key on Windows, a
// forced managed preference on macOS - must not override it in either
// direction while external policy is compiled out (#358).

#include <memory>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/components/growser_policy/buildflags/buildflags.h"
#include "components/crash/core/app/crash_reporter_client.h"
#include "components/crash/core/app/crashpad.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/crashpad/crashpad/client/crash_report_database.h"
#include "third_party/crashpad/crashpad/client/settings.h"

namespace {

// A crash client whose operating system enforces a policy value.
class PolicyEnforcingClient : public crash_reporter::CrashReporterClient {
 public:
  explicit PolicyEnforcingClient(bool policy_value)
      : policy_value_(policy_value) {}

  bool ReportingIsEnforcedByPolicy(bool* enabled) override {
    *enabled = policy_value_;
    return true;
  }
  bool IsRunningUnattended() override { return false; }

 private:
  const bool policy_value_;
};

class CrashpadPolicyTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    database_path_ = temp_dir_.GetPath();
    database_ = crashpad::CrashReportDatabase::Initialize(database_path_);
    ASSERT_TRUE(database_);
    crash_reporter::internal::SetCrashReportDatabaseForTesting(
        database_.get(), &database_path_);
  }

  void TearDown() override {
    crash_reporter::internal::SetCrashReportDatabaseForTesting(nullptr,
                                                               nullptr);
    crash_reporter::SetCrashReporterClient(nullptr);
  }

  // What Crashpad will do with a report after the user answered `consent`
  // while the operating system enforces `policy`.
  bool UploadsEnabled(bool policy, bool consent) {
    PolicyEnforcingClient client(policy);
    crash_reporter::SetCrashReporterClient(&client);
    crash_reporter::SetUploadConsent(consent);
    bool enabled = !consent;
    EXPECT_TRUE(database_->GetSettings()->GetUploadsEnabled(&enabled));
    crash_reporter::SetCrashReporterClient(nullptr);
    return enabled;
  }

 private:
  base::ScopedTempDir temp_dir_;
  base::FilePath database_path_;
  std::unique_ptr<crashpad::CrashReportDatabase> database_;
};

#if !BUILDFLAG(GROWSER_ENABLE_EXTERNAL_POLICY)
TEST_F(CrashpadPolicyTest, PolicyDoesNotTurnUploadsOnAgainstTheUser) {
  EXPECT_FALSE(UploadsEnabled(/*policy=*/true, /*consent=*/false));
}

TEST_F(CrashpadPolicyTest, PolicyDoesNotTurnUploadsOffAgainstTheUser) {
  EXPECT_TRUE(UploadsEnabled(/*policy=*/false, /*consent=*/true));
}
#endif

}  // namespace
