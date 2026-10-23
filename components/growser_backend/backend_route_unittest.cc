/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser#347: the route decides where every request to our backend goes, so
// these pin down both halves - which URLs move, and when the prober moves them.

#include "brave/components/growser_backend/backend_route.h"

#include <memory>

#include "base/test/task_environment.h"
#include "components/component_updater/configurator_impl.h"
#include "components/update_client/command_line_config_policy.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace growser_backend {

namespace {

constexpr char kPrimaryHealth[] = "https://backend.growser.org/healthz";
constexpr char kMirrorHealth[] = "https://ru.growser.org/healthz";

class BackendRouteTest : public testing::Test {
 protected:
  BackendRouteTest() {
    SetOnMirror(false);
    BackendProber::RegisterLocalStatePrefs(prefs_.registry());
  }
  ~BackendRouteTest() override { SetOnMirror(false); }

  std::unique_ptr<BackendProber> MakeProber() {
    return std::make_unique<BackendProber>(
        &prefs_, factory_.GetSafeWeakWrapper());
  }

  void Answer(const char* url, const char* body) {
    factory_.AddResponse(url, body);
  }

  void Fail(const char* url) {
    factory_.AddResponse(GURL(url), network::mojom::URLResponseHead::New(), "",
                         network::URLLoaderCompletionStatus(net::ERR_TIMED_OUT));
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  net::test::ScopedMockNetworkChangeNotifier network_change_notifier_;
  network::TestURLLoaderFactory factory_;
  TestingPrefServiceSimple prefs_;
};

}  // namespace

TEST_F(BackendRouteTest, OnThePrimaryNothingMoves) {
  const GURL url("https://backend.growser.org/v4/threatListUpdates:fetch");
  EXPECT_EQ(RouteUrl(url), url);
}

TEST_F(BackendRouteTest, OnTheMirrorOurHostsMoveWithPathAndQuery) {
  SetOnMirror(true);
  EXPECT_EQ(RouteUrl(GURL("https://backend.growser.org/v4/x:fetch?$req=1")),
            GURL("https://ru.growser.org/v4/x:fetch?$req=1"));
  EXPECT_EQ(RouteUrl(GURL("https://icon.growser.org/icon?domain=ya.ru")),
            GURL("https://ru.growser.org/icon?domain=ya.ru"));
}

TEST_F(BackendRouteTest, OnTheMirrorOtherHostsAndThePrimaryHealthStay) {
  SetOnMirror(true);
  for (const char* spec :
       {"https://growser.org/sparkle/stable-arm64/appcast.xml",
        "https://update.googleapis.com/service/update2/json",
        "https://notbackend.growser.org.example.com/v4/", kPrimaryHealth}) {
    EXPECT_EQ(RouteUrl(GURL(spec)), GURL(spec)) << spec;
  }
}

TEST_F(BackendRouteTest, APrimaryThatAnswersKeepsTheRoute) {
  Answer(kPrimaryHealth, kHealthAnswer);
  auto prober = MakeProber();
  prober->Start();
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(IsOnMirror());
  EXPECT_FALSE(factory_.IsPending(kMirrorHealth));
}

TEST_F(BackendRouteTest, ABlockedPrimaryMovesTheRouteToTheMirror) {
  Fail(kPrimaryHealth);
  Answer(kMirrorHealth, kHealthAnswer);
  auto prober = MakeProber();
  prober->Start();
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(IsOnMirror());
  EXPECT_TRUE(prefs_.GetBoolean(kOnMirrorPref));
}

TEST_F(BackendRouteTest, NeitherAnsweringMeansOfflineNotBlocked) {
  SetOnMirror(true);
  Fail(kPrimaryHealth);
  Fail(kMirrorHealth);
  auto prober = MakeProber();
  prober->Start();
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(IsOnMirror());
}

TEST_F(BackendRouteTest, AnAnswerThatIsNotOursCountsAsNoAnswer) {
  // A captive portal or a blocking page answers 200 to everything.
  Answer(kPrimaryHealth, "<html>login</html>");
  Answer(kMirrorHealth, kHealthAnswer);
  auto prober = MakeProber();
  prober->Start();
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(IsOnMirror());
}

TEST_F(BackendRouteTest, TheLastRouteIsWhereTheNextStartBegins) {
  prefs_.SetBoolean(kOnMirrorPref, true);
  auto prober = MakeProber();
  EXPECT_TRUE(IsOnMirror());
}

TEST_F(BackendRouteTest, TheRouteComesBackWhenThePrimaryDoes) {
  Fail(kPrimaryHealth);
  Answer(kMirrorHealth, kHealthAnswer);
  auto prober = MakeProber();
  prober->Start();
  task_environment_.RunUntilIdle();
  ASSERT_TRUE(IsOnMirror());

  Answer(kPrimaryHealth, kHealthAnswer);
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(IsOnMirror());
  EXPECT_FALSE(prefs_.GetBoolean(kOnMirrorPref));
}

TEST_F(BackendRouteTest, TheTimerAsksAgain) {
  Answer(kPrimaryHealth, kHealthAnswer);
  auto prober = MakeProber();
  prober->Start();
  task_environment_.RunUntilIdle();
  ASSERT_FALSE(IsOnMirror());

  Fail(kPrimaryHealth);
  Answer(kMirrorHealth, kHealthAnswer);
  task_environment_.FastForwardBy(base::Minutes(30));
  EXPECT_TRUE(IsOnMirror());
}

namespace {

class UrlSource : public update_client::CommandLineConfigPolicy {
 public:
  explicit UrlSource(const char* url) : url_(url) {}
  GURL UrlSourceOverride() const override { return url_; }

 private:
  GURL url_;
};

}  // namespace

// Components on every platform, iOS included, where the request hook does not
// run: update_client walks this list on a network error.
TEST(BackendComponentUrlsTest, OurUpdateUrlIsFollowedByTheMirror) {
  component_updater::ConfiguratorImpl configurator(
      UrlSource("https://backend.growser.org/components"), true);
  EXPECT_EQ(configurator.UpdateUrl(),
            (std::vector<GURL>{GURL("https://backend.growser.org/components"),
                               GURL("https://ru.growser.org/components")}));
}

TEST(BackendComponentUrlsTest, AnotherUpdateUrlIsLeftAlone) {
  component_updater::ConfiguratorImpl configurator(
      UrlSource("https://example.com/service/update2/json"), true);
  EXPECT_EQ(configurator.UpdateUrl(),
            (std::vector<GURL>{GURL("https://example.com/service/update2/json")}));
}

}  // namespace growser_backend
