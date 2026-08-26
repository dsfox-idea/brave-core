/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_UPDATER_H_
#define BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_UPDATER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"

class PrefRegistrySimple;
class PrefService;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace growser_ru_trust {

// Where the fetched list is kept. Local state, not a profile pref: the list
// is a property of the installation, not of who is browsing.
extern const char kRuTrustDomainsPref[];
extern const char kRuTrustVersionPref[];

// growser (#95): keeps the Russian CA domain allowlist current between
// releases.
//
// The build carries a list generated from CT logs, and it is exactly as old as
// the build. A site that moves to that root afterwards fails to open, and the
// user cannot tell that from an attack. This fetches a newer list, signed on
// the release machine, and keeps it in local state.
//
// Three things it deliberately does NOT do:
//   - trust an unsigned payload. The signature is checked against the key
//     below, which is compiled in; a server we run is not a reason to skip it,
//     because the payload decides what a state-controlled CA may vouch for.
//   - shrink the list. What arrives is merged with what the build carries, so
//     a truncated or hostile payload can only ever ADD domains, never remove
//     the ones a user already relies on.
//   - apply mid-session. The fetched list is read when profile prefs are
//     registered, so it takes effect on the next start. Rewriting certificate
//     trust under a running network context buys a day and costs a class of
//     bug we would rather not own.
class RuTrustUpdater {
 public:
  RuTrustUpdater(PrefService* local_state,
                 scoped_refptr<network::SharedURLLoaderFactory> loader);
  RuTrustUpdater(const RuTrustUpdater&) = delete;
  RuTrustUpdater& operator=(const RuTrustUpdater&) = delete;
  ~RuTrustUpdater();

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  // Fetches now and then once a day.
  void Start();

  // Verifies a payload the way Start() does. Exposed for tests, which is the
  // only way to check the rejection paths without a server.
  static bool VerifyForTesting(const std::string& payload,
                               const std::string& signature_base64);

 private:
  void Fetch();
  void OnFetched(std::optional<std::string> body);

  raw_ptr<PrefService> local_state_;
  scoped_refptr<network::SharedURLLoaderFactory> loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  base::RepeatingTimer timer_;
  base::WeakPtrFactory<RuTrustUpdater> weak_factory_{this};
};

}  // namespace growser_ru_trust

#endif  // BRAVE_COMPONENTS_GROWSER_RU_TRUST_RU_TRUST_UPDATER_H_
