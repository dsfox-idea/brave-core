// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/p3a_service.h"

#include <memory>

#include "base/metrics/histogram_functions.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/p3a_utils/custom_attributes.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

constexpr char kTestExpressHistogramName[] = "Brave.Core.UsageDaily";
constexpr char kTestSlowHistogramName[] = "Brave.Core.UsageMonthly";
constexpr char kTestTypicalHistogramName[] = "Brave.Core.IsDefault";

class P3AServiceTest : public testing::Test {
 public:
  P3AServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    P3AService::RegisterPrefs(local_state_.registry(), true);
  }

  void TearDown() override { p3a_service_ = nullptr; }

  void CreateP3AService() {
    base::Time install_time;
    ASSERT_TRUE(base::Time::FromString("2049-01-01", &install_time));
    p3a_service_ = scoped_refptr(
        new P3AService(local_state_, "release", install_time, {}));
    p3a_service_->Init(shared_url_loader_factory_, nullptr);
  }

  void TriggerRemoteConfigLoad() {
    // Set the remote config as loaded and trigger the callback
    p3a_service_->remote_config_manager()->SetIsLoadedForTesting(true);
    p3a_service_->OnRemoteConfigLoaded();
  }

  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  scoped_refptr<P3AService> p3a_service_;
  TestingPrefServiceSimple local_state_;
};

// growser (#21): P3A is neutralised rather than compiled out - Init(),
// InitCallbacks() and RegisterDynamicMetric() have empty bodies, so no histogram
// observer is ever installed and nothing is ever prepared for sending. Brave's
// versions of the three tests below asserted the opposite; inverted, they become
// the gate that keeps the removal from being undone quietly - including when the
// pref is forced on by hand, which is the case that matters.
TEST_F(P3AServiceTest, MessageManagerStaysInactiveEvenWhenP3AEnabled) {
  local_state_.SetBoolean(kP3AEnabled, true);
  CreateP3AService();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  TriggerRemoteConfigLoad();
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());
}

TEST_F(P3AServiceTest, MessageManagerNotStartedWhenP3ADisabled) {
  local_state_.SetBoolean(kP3AEnabled, false);
  CreateP3AService();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  TriggerRemoteConfigLoad();
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());
}

TEST_F(P3AServiceTest, MessageManagerIgnoresPrefChanges) {
  local_state_.SetBoolean(kP3AEnabled, false);
  CreateP3AService();

  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  TriggerRemoteConfigLoad();
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  // Turning the pref on used to start the manager. It must not any more.
  local_state_.SetBoolean(kP3AEnabled, true);
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());

  local_state_.SetBoolean(kP3AEnabled, false);
  EXPECT_FALSE(p3a_service_->message_manager_->IsActive());
}

TEST_F(P3AServiceTest, MetricValueNotStored) {
  local_state_.SetBoolean(kP3AEnabled, true);

  CreateP3AService();

  base::UmaHistogramExactLinear(kTestTypicalHistogramName, 0, 10);
  p3a_service_->OnHistogramChanged(kTestTypicalHistogramName, 1, 0);
  base::UmaHistogramExactLinear(kTestExpressHistogramName, 0, 10);
  p3a_service_->OnHistogramChanged(kTestExpressHistogramName, 1, 0);
  base::UmaHistogramExactLinear(kTestSlowHistogramName, 0, 10);
  p3a_service_->OnHistogramChanged(kTestSlowHistogramName, 1, 0);
  task_environment_.FastForwardBy(base::Seconds(3));

  TriggerRemoteConfigLoad();
  task_environment_.FastForwardBy(base::Seconds(3));

  // Nothing is prepared for sending, in any cadence, at any point.
  EXPECT_TRUE(local_state_.GetDict(kTypicalConstellationPrepPrefName).empty());
  EXPECT_TRUE(local_state_.GetDict(kExpressConstellationPrepPrefName).empty());
  EXPECT_TRUE(local_state_.GetDict(kSlowConstellationPrepPrefName).empty());
}

TEST_F(P3AServiceTest, CustomAttributeStored) {
  CreateP3AService();

  p3a_utils::SetCustomAttribute("foo", "bar");

  const auto* value =
      local_state_.GetDict(kCustomAttributesDictPref).FindString("foo");
  ASSERT_TRUE(value != nullptr);
  EXPECT_EQ(*value, "bar");

  p3a_utils::SetCustomAttribute("foo", std::nullopt);
  EXPECT_FALSE(
      local_state_.GetDict(kCustomAttributesDictPref).FindString("foo"));
}

}  // namespace p3a
