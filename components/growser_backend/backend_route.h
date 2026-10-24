/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GROWSER_BACKEND_BACKEND_ROUTE_H_
#define BRAVE_COMPONENTS_GROWSER_BACKEND_BACKEND_ROUTE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace growser_backend {

// growser#347: some networks cannot reach backend.growser.org (Cloudflare)
// while the mirror ru.growser.org, which relays to the same Worker, answers.
// The browser's own requests to our backend follow the route BackendProber
// last measured.
inline constexpr char kPrimaryHost[] = "backend.growser.org";
inline constexpr char kIconHost[] = "icon.growser.org";
inline constexpr char kMirrorHost[] = "ru.growser.org";
inline constexpr char kHealthPath[] = "/healthz";
inline constexpr char kHealthAnswer[] = "growser backend";
// Local state: the route found last time, so a browser on a blocked network
// starts on the mirror instead of timing out until the first answer.
inline constexpr char kOnMirrorPref[] = "growser.backend.on_mirror";

bool IsOnMirror();
void SetOnMirror(bool on_mirror);

// `url` moved to the mirror when the route is on the mirror and `url` goes to
// our backend; `url` itself otherwise. The primary's health check is never
// moved: it is how the route finds its way back.
GURL RouteUrl(const GURL& url);

// Asks the primary's /healthz and, when it does not answer, the mirror's, at
// start, every 30 minutes and whenever the network changes. Neither answering
// leaves the route on the primary: the mirror is for a primary that is
// blocked, not for a machine that is offline.
class BackendProber : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  BackendProber(PrefService* local_state,
                scoped_refptr<network::SharedURLLoaderFactory> loader_factory);
  BackendProber(const BackendProber&) = delete;
  BackendProber& operator=(const BackendProber&) = delete;
  ~BackendProber() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  void Start();
  void Probe();

 private:
  void Ask(const std::string& host);
  void OnAnswered(const std::string& host, std::optional<std::string> body);

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  raw_ptr<PrefService> local_state_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  base::RepeatingTimer timer_;
  base::WeakPtrFactory<BackendProber> weak_factory_{this};
};

}  // namespace growser_backend

#endif  // BRAVE_COMPONENTS_GROWSER_BACKEND_BACKEND_ROUTE_H_
