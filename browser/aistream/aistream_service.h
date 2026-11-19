/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AISTREAM_AISTREAM_SERVICE_H_
#define BRAVE_BROWSER_AISTREAM_AISTREAM_SERVICE_H_

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace base {
class CommandLine;
class Environment;
}  // namespace base

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace aistream {

struct Widget {
  std::string id;
  std::string title;
  std::string markup;
};

// growser#422: the browser side of the New Tab Page widget bridge
// (tools/aistream-bridge in the control repo). One per regular profile, and
// only when the browser was started with --aistream-bridge. It holds the
// bridge's SSE stream of markup updates, reconnecting while the bridge is
// away, and forwards widget messages to it.
class AistreamService : public KeyedService {
 public:
  struct Config {
    GURL bridge;
    std::string token;
  };

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnWidgetMarkup(const std::string& widget_id,
                                const std::string& markup) = 0;
  };

  using WidgetsCallback = base::OnceCallback<void(std::vector<Widget>)>;
  // The new markup, or a reason fit to show the person.
  using MessageCallback =
      base::OnceCallback<void(base::expected<std::string, std::string>)>;
  // The new widget, or a reason fit to show the person.
  using AskCallback =
      base::OnceCallback<void(base::expected<Widget, std::string>)>;

  static constexpr char kBridgeSwitch[] = "aistream-bridge";
  static constexpr char kTokenVariable[] = "AISTREAM_TOKEN";
  static constexpr base::TimeDelta kReconnectDelay = base::Seconds(5);
  // The agent behind the bridge may think for a while.
  static constexpr base::TimeDelta kMessageTimeout = base::Minutes(5);

  // The bridge address from the switch (http, this machine only) and the
  // token from the environment; nullopt, with the reason logged, otherwise.
  static std::optional<Config> ReadConfig(const base::CommandLine& command_line,
                                          base::Environment& environment);

  static bool IsValidWidgetId(std::string_view widget_id);

  AistreamService(
      Config config,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  AistreamService(const AistreamService&) = delete;
  AistreamService& operator=(const AistreamService&) = delete;
  ~AistreamService() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void GetWidgets(WidgetsCallback callback);
  void SendMessage(const std::string& widget_id,
                   const std::string& message,
                   const std::string& dom,
                   MessageCallback callback);
  void Ask(const std::string& query,
           const std::string& dom,
           AskCallback callback);

 private:
  base::flat_map<std::string, std::string> Headers() const;
  GURL Endpoint(std::string_view path) const;

  void OpenEvents();
  void OnEvent(api_request_helper::ValueOrError event);
  void OnEventsClosed(api_request_helper::APIRequestResult result);
  void OnWidgets(WidgetsCallback callback,
                 api_request_helper::APIRequestResult result);
  void OnMessageReply(const std::string& widget_id,
                      MessageCallback callback,
                      api_request_helper::APIRequestResult result);
  void OnAskReply(AskCallback callback,
                  api_request_helper::APIRequestResult result);

  const Config config_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::OneShotTimer reconnect_timer_;
  // The (HTTP status, net error) of the last close that was logged, so a
  // bridge that stays away is reported once rather than every few seconds.
  std::optional<std::pair<int, int>> logged_close_;
  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<AistreamService> weak_ptr_factory_{this};
};

}  // namespace aistream

#endif  // BRAVE_BROWSER_AISTREAM_AISTREAM_SERVICE_H_
