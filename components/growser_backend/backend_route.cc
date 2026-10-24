/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/growser_backend/backend_route.h"

#include <atomic>
#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace growser_backend {

namespace {

// Read on whichever thread a request starts on, written by the prober.
std::atomic<bool> g_on_mirror{false};

constexpr base::TimeDelta kInterval = base::Minutes(30);
constexpr base::TimeDelta kTimeout = base::Seconds(10);
constexpr size_t kMaxBodyBytes = 64;

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("growser_backend_health", R"(
      semantics {
        sender: "Growser backend route"
        description:
          "Asks whether the Growser backend answers on this network, and when "
          "it does not, whether its auxiliary server on the same domain does, "
          "so the browser's own services keep working."
        trigger: "At startup, every 30 minutes and when the network changes."
        data: "None. The request carries no identifiers."
        destination: OTHER
        destination_other: "backend.growser.org and ru.growser.org, operated "
          "by the Growser project"
      }
      policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled by settings."
        policy_exception_justification: "Not implemented."
      })");

}  // namespace

bool IsOnMirror() {
  return g_on_mirror.load(std::memory_order_relaxed);
}

void SetOnMirror(bool on_mirror) {
  g_on_mirror.store(on_mirror, std::memory_order_relaxed);
}

GURL RouteUrl(const GURL& url) {
  if (!IsOnMirror() ||
      !(url.host() == kPrimaryHost || url.host() == kIconHost) ||
      (url.host() == kPrimaryHost && url.path() == kHealthPath)) {
    return url;
  }
  GURL::Replacements replacements;
  replacements.SetSchemeStr("https");
  replacements.SetHostStr(kMirrorHost);
  return url.ReplaceComponents(replacements);
}

BackendProber::BackendProber(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory)
    : local_state_(local_state), loader_factory_(std::move(loader_factory)) {
  if (local_state_) {
    SetOnMirror(local_state_->GetBoolean(kOnMirrorPref));
  }
}

BackendProber::~BackendProber() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

// static
void BackendProber::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kOnMirrorPref, false);
}

void BackendProber::Start() {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  Probe();
  timer_.Start(FROM_HERE, kInterval,
               base::BindRepeating(&BackendProber::Probe,
                                   weak_factory_.GetWeakPtr()));
}

void BackendProber::Probe() {
  if (!loader_factory_ || loader_) {
    return;
  }
  Ask(kPrimaryHost);
}

void BackendProber::Ask(const std::string& host) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(base::StrCat({"https://", host, kHealthPath}));
  request->method = "GET";
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  loader_ = network::SimpleURLLoader::Create(std::move(request),
                                             kTrafficAnnotation);
  loader_->SetTimeoutDuration(kTimeout);
  loader_->DownloadToString(
      loader_factory_.get(),
      base::BindOnce(&BackendProber::OnAnswered, weak_factory_.GetWeakPtr(),
                     host),
      kMaxBodyBytes);
}

void BackendProber::OnAnswered(const std::string& host,
                               std::optional<std::string> body) {
  loader_.reset();
  // The exact body, not any 200: a captive portal answers everything.
  const bool healthy = body && *body == kHealthAnswer;
  const bool asked_primary = host == kPrimaryHost;
  if (asked_primary && !healthy) {
    Ask(kMirrorHost);
    return;
  }
  const bool on_mirror = !asked_primary && healthy;
  if (on_mirror != IsOnMirror()) {
    LOG(WARNING) << "growser backend: " << kPrimaryHost
                 << (on_mirror ? " does not answer, using " : " answers, leaving ")
                 << kMirrorHost;
  }
  SetOnMirror(on_mirror);
  if (local_state_) {
    local_state_->SetBoolean(kOnMirrorPref, on_mirror);
  }
}

void BackendProber::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (type != net::NetworkChangeNotifier::CONNECTION_NONE) {
    Probe();
  }
}

}  // namespace growser_backend
