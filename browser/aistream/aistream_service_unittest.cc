/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/aistream/aistream_service.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/json/json_reader.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace aistream {

namespace {

constexpr char kBridge[] = "http://127.0.0.1:8650/";
constexpr char kEvents[] = "http://127.0.0.1:8650/events";
constexpr char kWidgets[] = "http://127.0.0.1:8650/widgets";
constexpr char kNoteMessage[] = "http://127.0.0.1:8650/widgets/note/message";

class FakeEnvironment : public base::Environment {
 public:
  explicit FakeEnvironment(std::optional<std::string> token)
      : token_(std::move(token)) {}

  std::optional<std::string> GetVar(base::cstring_view name) override {
    return std::string_view(name) == AistreamService::kTokenVariable
               ? token_
               : std::nullopt;
  }
  bool SetVar(base::cstring_view, const std::string&) override {
    return false;
  }
  bool UnSetVar(base::cstring_view) override { return false; }

 private:
  std::optional<std::string> token_;
};

class RecordingObserver : public AistreamService::Observer {
 public:
  void OnWidgetMarkup(const std::string& widget_id,
                      const std::string& markup) override {
    updates.emplace_back(widget_id, markup);
  }

  std::vector<std::pair<std::string, std::string>> updates;
};

std::optional<AistreamService::Config> ReadConfig(
    const std::vector<std::string>& switches,
    std::optional<std::string> token) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  for (const auto& value : switches) {
    command_line.AppendSwitchASCII(AistreamService::kBridgeSwitch, value);
  }
  FakeEnvironment environment(std::move(token));
  return AistreamService::ReadConfig(command_line, environment);
}

}  // namespace

TEST(AistreamServiceConfigTest, OffWithoutTheSwitch) {
  EXPECT_FALSE(ReadConfig({}, "secret"));
}

TEST(AistreamServiceConfigTest, OnlyPlainHttpOnThisMachine) {
  EXPECT_FALSE(ReadConfig({"https://127.0.0.1:8650"}, "secret"));
  EXPECT_FALSE(ReadConfig({"http://example.com:8650"}, "secret"));
  EXPECT_FALSE(ReadConfig({"http://10.0.0.5:8650"}, "secret"));
  EXPECT_FALSE(ReadConfig({"not a url"}, "secret"));

  auto config = ReadConfig({"http://127.0.0.1:8650/some/path"}, "secret");
  ASSERT_TRUE(config);
  EXPECT_EQ(config->bridge, GURL(kBridge));
  EXPECT_EQ(config->token, "secret");
  EXPECT_TRUE(ReadConfig({"http://localhost:8650"}, "secret"));
}

TEST(AistreamServiceConfigTest, OffWithoutAToken) {
  EXPECT_FALSE(ReadConfig({kBridge}, std::nullopt));
  EXPECT_FALSE(ReadConfig({kBridge}, ""));
}

TEST(AistreamServiceConfigTest, WidgetIdsCannotReshapeThePath) {
  EXPECT_TRUE(AistreamService::IsValidWidgetId("note_1-a"));
  EXPECT_FALSE(AistreamService::IsValidWidgetId(""));
  EXPECT_FALSE(AistreamService::IsValidWidgetId("../events"));
  EXPECT_FALSE(AistreamService::IsValidWidgetId("a/b"));
  EXPECT_FALSE(AistreamService::IsValidWidgetId("a?b"));
}

class AistreamServiceTest : public testing::Test {
 protected:
  void SetUp() override {
    service_ = std::make_unique<AistreamService>(
        AistreamService::Config{GURL(kBridge), "secret"},
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_));
    service_->AddObserver(&observer_);
  }

  void TearDown() override { service_->RemoveObserver(&observer_); }

  // Answers the open event stream with `body` and closes it.
  void AnswerEvents(const std::string& body) {
    auto head = network::CreateURLResponseHead(net::HTTP_OK);
    head->mime_type = "text/event-stream";
    ASSERT_TRUE(url_loader_factory_.SimulateResponseForPendingRequest(
        GURL(kEvents), network::URLLoaderCompletionStatus(net::OK),
        std::move(head), body));
    task_environment_.RunUntilIdle();
  }

  std::string AuthorizationOf(const char* url) {
    const network::ResourceRequest* request = nullptr;
    EXPECT_TRUE(url_loader_factory_.IsPending(url, &request));
    return request ? request->headers
                         .GetHeader(net::HttpRequestHeaders::kAuthorization)
                         .value_or("")
                   : "";
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  network::TestURLLoaderFactory url_loader_factory_;
  RecordingObserver observer_;
  std::unique_ptr<AistreamService> service_;
};

TEST_F(AistreamServiceTest, OpensTheEventStreamWithTheToken) {
  EXPECT_EQ(AuthorizationOf(kEvents), "Bearer secret");
}

TEST_F(AistreamServiceTest, AnEventReachesTheObservers) {
  AnswerEvents(
      ": open\n\n"
      "data: {\"widget\": \"note\", \"markup\": \"<p>hi</p>\"}\n\n"
      "data: {\"widget\": \"../x\", \"markup\": \"<p>bad id</p>\"}\n\n"
      "data: {\"widget\": \"note\"}\n\n");
  ASSERT_EQ(observer_.updates.size(), 1u);
  EXPECT_EQ(observer_.updates[0].first, "note");
  EXPECT_EQ(observer_.updates[0].second, "<p>hi</p>");
}

TEST_F(AistreamServiceTest, AClosedStreamReconnectsAfterTheDelay) {
  url_loader_factory_.SimulateResponseForPendingRequest(
      kEvents, "", net::HTTP_SERVICE_UNAVAILABLE);
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(url_loader_factory_.IsPending(kEvents));

  task_environment_.FastForwardBy(AistreamService::kReconnectDelay -
                                  base::Milliseconds(1));
  EXPECT_FALSE(url_loader_factory_.IsPending(kEvents));
  task_environment_.FastForwardBy(base::Milliseconds(1));
  EXPECT_TRUE(url_loader_factory_.IsPending(kEvents));

  // The stream that comes back still delivers.
  AnswerEvents("data: {\"widget\": \"note\", \"markup\": \"again\"}\n\n");
  ASSERT_EQ(observer_.updates.size(), 1u);
  EXPECT_EQ(observer_.updates[0].second, "again");
}

TEST_F(AistreamServiceTest, ListsTheWidgetsWithValidIds) {
  base::test::TestFuture<std::vector<Widget>> future;
  service_->GetWidgets(future.GetCallback());
  EXPECT_EQ(AuthorizationOf(kWidgets), "Bearer secret");
  url_loader_factory_.SimulateResponseForPendingRequest(
      kWidgets,
      R"([{"id": "note", "title": "Note", "markup": "<p>x</p>"},
          {"id": "a/b", "title": "Bad"},
          {"title": "No id"},
          {"id": "bare"}])");
  std::vector<Widget> widgets = future.Take();
  ASSERT_EQ(widgets.size(), 2u);
  EXPECT_EQ(widgets[0].id, "note");
  EXPECT_EQ(widgets[0].title, "Note");
  EXPECT_EQ(widgets[0].markup, "<p>x</p>");
  EXPECT_EQ(widgets[1].id, "bare");
  EXPECT_EQ(widgets[1].title, "");
}

TEST_F(AistreamServiceTest, NoWidgetsWhenTheBridgeFails) {
  base::test::TestFuture<std::vector<Widget>> future;
  service_->GetWidgets(future.GetCallback());
  url_loader_factory_.SimulateResponseForPendingRequest(
      kWidgets, R"({"error": "unauthorized"})", net::HTTP_UNAUTHORIZED);
  EXPECT_TRUE(future.Take().empty());
}

TEST_F(AistreamServiceTest, AMessageCarriesTheDomAndReturnsTheMarkup) {
  base::test::TestFuture<base::expected<std::string, std::string>> future;
  service_->SendMessage("note", "hello", "<main>ntp</main>",
                        future.GetCallback());

  const network::ResourceRequest* request = nullptr;
  ASSERT_TRUE(url_loader_factory_.IsPending(kNoteMessage, &request));
  EXPECT_EQ(request->method, "POST");
  EXPECT_EQ(AuthorizationOf(kNoteMessage), "Bearer secret");
  std::optional<base::Value> body =
      base::JSONReader::Read(network::GetUploadData(*request), 0);
  ASSERT_TRUE(body && body->is_dict());
  EXPECT_EQ(*body->GetDict().FindString("message"), "hello");
  EXPECT_EQ(*body->GetDict().FindString("dom"), "<main>ntp</main>");

  url_loader_factory_.SimulateResponseForPendingRequest(
      kNoteMessage, R"({"markup": "<p>answer</p>"})");
  auto reply = future.Take();
  ASSERT_TRUE(reply.has_value());
  EXPECT_EQ(reply.value(), "<p>answer</p>");
}

TEST_F(AistreamServiceTest, AFailedMessageSaysWhy) {
  base::test::TestFuture<base::expected<std::string, std::string>> future;
  service_->SendMessage("note", "hello", "", future.GetCallback());
  url_loader_factory_.SimulateResponseForPendingRequest(
      kNoteMessage, R"({"error": "harness: HTTP 500"})", net::HTTP_BAD_GATEWAY);
  auto reply = future.Take();
  ASSERT_FALSE(reply.has_value());
  EXPECT_EQ(reply.error(), "harness: HTTP 500");

  base::test::TestFuture<base::expected<std::string, std::string>> unreachable;
  service_->SendMessage("note", "hello", "", unreachable.GetCallback());
  url_loader_factory_.SimulateResponseForPendingRequest(
      GURL(kNoteMessage),
      network::URLLoaderCompletionStatus(net::ERR_CONNECTION_REFUSED),
      network::CreateURLResponseHead(net::HTTP_OK), "");
  reply = unreachable.Take();
  ASSERT_FALSE(reply.has_value());
  EXPECT_EQ(reply.error(), "the bridge is not reachable");
}

TEST_F(AistreamServiceTest, AnInvalidWidgetIdSendsNothing) {
  base::test::TestFuture<base::expected<std::string, std::string>> future;
  service_->SendMessage("../events", "hello", "", future.GetCallback());
  auto reply = future.Take();
  ASSERT_FALSE(reply.has_value());
  // Only the event stream is pending.
  EXPECT_EQ(url_loader_factory_.NumPending(), 1);
}

}  // namespace aistream
