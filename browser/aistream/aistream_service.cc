/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/aistream/aistream_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/url_constants.h"

namespace aistream {

namespace {

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("growser_aistream_bridge", R"(
      semantics {
        sender: "Growser New Tab Page widgets"
        description:
          "Talks to a widget bridge the person runs on their own machine: "
          "lists the widgets, sends a widget's message with the New Tab "
          "Page's content, and listens for widget updates."
        trigger:
          "Starting the browser with --aistream-bridge, then opening a new "
          "tab or sending a widget message."
        data:
          "The widget message and the New Tab Page's DOM, including the top "
          "sites. Sent only to the address given on the command line, which "
          "must be this machine."
        destination: LOCAL
      }
      policy {
        cookies_allowed: NO
        setting: "Starting the browser without --aistream-bridge stops this."
        policy_exception_justification: "An experiment, never on by default."
      })");

constexpr char kJsonType[] = "application/json";

// Why a request to the bridge failed, in words fit to show the person.
std::string ReplyError(const api_request_helper::APIRequestResult& result) {
  const base::DictValue* body = result.value_body().GetIfDict();
  const std::string* reason = body ? body->FindString("error") : nullptr;
  if (reason) {
    return *reason;
  }
  return result.response_code() > 0
             ? base::StrCat(
                   {"HTTP ", base::NumberToString(result.response_code())})
             : std::string("the bridge is not reachable");
}

}  // namespace

// static
std::optional<AistreamService::Config> AistreamService::ReadConfig(
    const base::CommandLine& command_line,
    base::Environment& environment) {
  if (!command_line.HasSwitch(kBridgeSwitch)) {
    return std::nullopt;
  }
  GURL bridge(command_line.GetSwitchValueASCII(kBridgeSwitch));
  if (!bridge.is_valid() || !bridge.SchemeIs(url::kHttpScheme) ||
      !net::IsLocalhost(bridge)) {
    LOG(ERROR) << "aistream: --" << kBridgeSwitch
               << " must be an http address on this machine, got '"
               << bridge.possibly_invalid_spec() << "'; widgets are off";
    return std::nullopt;
  }
  std::optional<std::string> token = environment.GetVar(kTokenVariable);
  if (!token || token->empty()) {
    LOG(ERROR) << "aistream: " << kTokenVariable
               << " is not set; widgets are off";
    return std::nullopt;
  }
  return Config{bridge.GetWithEmptyPath(), *token};
}

// static
bool AistreamService::IsValidWidgetId(std::string_view widget_id) {
  // The bridge's own rule (bridge.py widget_route), so an id can never
  // reshape the path it is put into.
  if (widget_id.empty()) {
    return false;
  }
  for (char character : widget_id) {
    if (!base::IsAsciiAlphaNumeric(character) && character != '_' &&
        character != '-') {
      return false;
    }
  }
  return true;
}

AistreamService::AistreamService(
    Config config,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : config_(std::move(config)),
      api_request_helper_(kTrafficAnnotation, std::move(url_loader_factory)) {
  LOG(WARNING) << "aistream: widgets on, bridge " << config_.bridge.spec();
  OpenEvents();
}

AistreamService::~AistreamService() = default;

void AistreamService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AistreamService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

base::flat_map<std::string, std::string> AistreamService::Headers() const {
  return {{net::HttpRequestHeaders::kAuthorization,
           base::StrCat({"Bearer ", config_.token})}};
}

GURL AistreamService::Endpoint(std::string_view path) const {
  return config_.bridge.Resolve(path);
}

void AistreamService::OpenEvents() {
  api_request_helper_.RequestSSE(
      "GET", Endpoint("/events"), std::string(), std::string(),
      base::BindRepeating(&AistreamService::OnEvent,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&AistreamService::OnEventsClosed,
                     weak_ptr_factory_.GetWeakPtr()),
      Headers(), {});
}

void AistreamService::OnEvent(api_request_helper::ValueOrError event) {
  if (!event.has_value() || !event->is_dict()) {
    LOG(ERROR) << "aistream: an event that is not a JSON object: "
               << (event.has_value() ? "" : event.error());
    return;
  }
  const std::string* widget_id = event->GetDict().FindString("widget");
  const std::string* markup = event->GetDict().FindString("markup");
  if (!widget_id || !markup || !IsValidWidgetId(*widget_id)) {
    LOG(ERROR) << "aistream: an event without a valid widget and markup";
    return;
  }
  // A working stream is news worth logging the next time it closes.
  logged_close_.reset();
  VLOG(1) << "aistream: markup for " << *widget_id << ", " << markup->size()
          << " chars";
  for (auto& observer : observers_) {
    observer.OnWidgetMarkup(*widget_id, *markup);
  }
}

void AistreamService::OnEventsClosed(
    api_request_helper::APIRequestResult result) {
  std::pair<int, int> close{result.response_code(), result.error_code()};
  if (logged_close_ != close) {
    LOG(WARNING) << "aistream: the event stream closed (HTTP "
                 << result.response_code() << ", net error "
                 << result.error_code() << "); reconnecting every "
                 << kReconnectDelay.InSeconds() << " s";
    logged_close_ = close;
  }
  reconnect_timer_.Start(FROM_HERE, kReconnectDelay,
                         base::BindOnce(&AistreamService::OpenEvents,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void AistreamService::GetWidgets(WidgetsCallback callback) {
  api_request_helper_.Request(
      "GET", Endpoint("/widgets"), std::string(), std::string(),
      base::BindOnce(&AistreamService::OnWidgets,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      Headers());
}

void AistreamService::OnWidgets(WidgetsCallback callback,
                                api_request_helper::APIRequestResult result) {
  std::vector<Widget> widgets;
  if (!result.Is2XXResponseCode() || !result.value_body().is_list()) {
    LOG(ERROR) << "aistream: GET /widgets failed (HTTP "
               << result.response_code() << ", net error "
               << result.error_code() << ")";
    std::move(callback).Run(std::move(widgets));
    return;
  }
  for (const base::Value& entry : result.value_body().GetList()) {
    const base::DictValue* dict = entry.GetIfDict();
    const std::string* id = dict ? dict->FindString("id") : nullptr;
    if (!id || !IsValidWidgetId(*id)) {
      LOG(ERROR) << "aistream: skipped a widget without a valid id";
      continue;
    }
    const std::string* title = dict->FindString("title");
    const std::string* markup = dict->FindString("markup");
    widgets.push_back(
        {*id, title ? *title : std::string(), markup ? *markup : std::string()});
  }
  std::move(callback).Run(std::move(widgets));
}

void AistreamService::SendMessage(const std::string& widget_id,
                                  const std::string& message,
                                  const std::string& dom,
                                  MessageCallback callback) {
  if (!IsValidWidgetId(widget_id)) {
    std::move(callback).Run(base::unexpected("unknown widget"));
    return;
  }
  std::optional<std::string> body = base::WriteJson(
      base::DictValue().Set("message", message).Set("dom", dom));
  CHECK(body);
  api_request_helper::APIRequestOptions options;
  options.timeout = kMessageTimeout;
  api_request_helper_.Request(
      "POST", Endpoint(base::StrCat({"/widgets/", widget_id, "/message"})),
      *body, kJsonType,
      base::BindOnce(&AistreamService::OnMessageReply,
                     weak_ptr_factory_.GetWeakPtr(), widget_id,
                     std::move(callback)),
      Headers(), options);
}

void AistreamService::OnMessageReply(
    const std::string& widget_id,
    MessageCallback callback,
    api_request_helper::APIRequestResult result) {
  const base::DictValue* body = result.value_body().GetIfDict();
  if (result.Is2XXResponseCode() && body) {
    if (const std::string* markup = body->FindString("markup")) {
      std::move(callback).Run(base::ok(*markup));
      return;
    }
  }
  std::string error = ReplyError(result);
  LOG(ERROR) << "aistream: message to " << widget_id << " failed: " << error
             << " (net error " << result.error_code() << ")";
  std::move(callback).Run(base::unexpected(std::move(error)));
}

void AistreamService::Ask(const std::string& query,
                          const std::string& dom,
                          AskCallback callback) {
  std::optional<std::string> body =
      base::WriteJson(base::DictValue().Set("query", query).Set("dom", dom));
  CHECK(body);
  api_request_helper::APIRequestOptions options;
  options.timeout = kMessageTimeout;
  api_request_helper_.Request(
      "POST", Endpoint("/ask"), *body, kJsonType,
      base::BindOnce(&AistreamService::OnAskReply,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      Headers(), options);
}

void AistreamService::OnAskReply(AskCallback callback,
                                 api_request_helper::APIRequestResult result) {
  const base::DictValue* body = result.value_body().GetIfDict();
  if (result.Is2XXResponseCode() && body) {
    const std::string* id = body->FindString("id");
    const std::string* title = body->FindString("title");
    const std::string* markup = body->FindString("markup");
    if (id && IsValidWidgetId(*id) && title && markup) {
      std::move(callback).Run(base::ok(Widget{*id, *title, *markup}));
      return;
    }
  }
  std::string error = result.Is2XXResponseCode()
                          ? std::string("the bridge sent no widget")
                          : ReplyError(result);
  LOG(ERROR) << "aistream: a question failed: " << error << " (net error "
             << result.error_code() << ")";
  std::move(callback).Run(base::unexpected(std::move(error)));
}

}  // namespace aistream
