/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/growser_ru_trust/ru_trust_updater.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "crypto/signature_verifier.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace growser_ru_trust {

namespace {

// Our own backend, the same host the redirector uses. Not derived from the
// redirector build arg on purpose: this is one constant, and a URL that can be
// pointed elsewhere by a build flag is a URL that can be pointed elsewhere.
constexpr char kListUrl[] = "https://backend.growser.org/ru-trust";
constexpr char kSignatureHeader[] = "x-ru-trust-signature";

// The list is ~100 KB of JSON. The cap is generous and still refuses to buffer
// something that is clearly not our payload.
constexpr size_t kMaxBodyBytes = 2 * 1024 * 1024;

constexpr base::TimeDelta kInterval = base::Hours(24);

// SPKI of the ECDSA P-256 public key whose private half signs the list on the
// release machine (../growser-keys/ru-trust.der). A payload that does not
// verify against exactly this is discarded: the fetch happens over TLS from a
// host we run, and neither of those facts is a reason to skip the check,
// because this payload decides what a state-controlled CA may vouch for.
constexpr uint8_t kPublicKeySpki[] = {
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
    0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
    0x42, 0x00, 0x04, 0x1b, 0x74, 0xce, 0x92, 0x20, 0x5a, 0x82, 0x45, 0xe3,
    0x1e, 0xa6, 0xe3, 0x31, 0xfc, 0x36, 0x13, 0x02, 0xf8, 0xd7, 0xef, 0x6e,
    0x69, 0xf4, 0xb7, 0x6f, 0xf9, 0xb5, 0x93, 0xca, 0xed, 0xb5, 0xbb, 0xb8,
    0xb6, 0xf1, 0xcc, 0x92, 0x7b, 0x97, 0x5a, 0x42, 0xbd, 0x68, 0x41, 0xf7,
    0xb7, 0x70, 0xef, 0x17, 0x13, 0xab, 0x12, 0x10, 0xcd, 0xaa, 0x27, 0x68,
    0x28, 0xe8, 0x53, 0xcb, 0x50, 0x07, 0x9b,
};

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("growser_ru_trust_list", R"(
      semantics {
        sender: "Growser RU trust list"
        description:
          "Downloads the list of domains for which the browser trusts the "
          "Russian national certification authority. The list ships with the "
          "build; this keeps it current between releases."
        trigger: "Once at startup and once a day afterwards."
        data: "None. The request carries no identifiers."
        destination: OTHER
        destination_other: "backend.growser.org, operated by the Growser project"
      }
      policy {
        cookies_allowed: NO
        setting: "This feature cannot be disabled by settings."
        policy_exception_justification: "Not implemented."
      })");

bool VerifySignature(std::string_view payload, std::string_view signature_b64) {
  std::optional<std::vector<uint8_t>> signature =
      base::Base64Decode(signature_b64);
  if (!signature || signature->empty()) {
    return false;
  }
  crypto::SignatureVerifier verifier;
  if (!verifier.VerifyInit(crypto::SignatureVerifier::ECDSA_SHA256, *signature,
                           kPublicKeySpki)) {
    return false;
  }
  verifier.VerifyUpdate(base::as_byte_span(payload));
  return verifier.VerifyFinal();
}

}  // namespace

// Local state rather than a profile pref: the list is a property of the
// installation, not of who is browsing.
const char kRuTrustDomainsPref[] = "growser.ru_trust.domains";
const char kRuTrustVersionPref[] = "growser.ru_trust.version";

RuTrustUpdater::RuTrustUpdater(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> loader)
    : local_state_(local_state), loader_factory_(std::move(loader)) {}

RuTrustUpdater::~RuTrustUpdater() = default;

// static
void RuTrustUpdater::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kRuTrustDomainsPref);
  registry->RegisterIntegerPref(kRuTrustVersionPref, 0);
}

// static
bool RuTrustUpdater::VerifyForTesting(const std::string& payload,
                                      const std::string& signature_base64) {
  return VerifySignature(payload, signature_base64);
}

void RuTrustUpdater::Start() {
  Fetch();
  timer_.Start(FROM_HERE, kInterval,
               base::BindRepeating(&RuTrustUpdater::Fetch,
                                   weak_factory_.GetWeakPtr()));
}

void RuTrustUpdater::Fetch() {
  if (!loader_factory_ || loader_) {
    return;
  }
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(kListUrl);
  request->method = "GET";
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  loader_ = network::SimpleURLLoader::Create(std::move(request),
                                             kTrafficAnnotation);
  loader_->DownloadToString(
      loader_factory_.get(),
      base::BindOnce(&RuTrustUpdater::OnFetched, weak_factory_.GetWeakPtr()),
      kMaxBodyBytes);
}

void RuTrustUpdater::OnFetched(std::optional<std::string> body) {
  std::unique_ptr<network::SimpleURLLoader> loader = std::move(loader_);
  if (!body || body->empty()) {
    VLOG(1) << "ru-trust: no list fetched; keeping the one we have";
    return;
  }
  std::optional<std::string> signature;
  if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
    signature =
        loader->ResponseInfo()->headers->GetNormalizedHeader(kSignatureHeader);
  }
  if (!signature) {
    LOG(WARNING) << "ru-trust: served without a signature header; discarded";
    return;
  }
  if (!VerifySignature(*body, *signature)) {
    // Loud, not silent: this means either a key rotation nobody finished or
    // something between us and the user rewriting the payload.
    LOG(ERROR) << "ru-trust: signature does not verify; discarded";
    return;
  }

  std::optional<base::Value> parsed =
      base::JSONReader::Read(*body, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed || !parsed->is_dict()) {
    LOG(ERROR) << "ru-trust: payload is not a JSON object; discarded";
    return;
  }
  const base::DictValue& document = parsed->GetDict();
  std::optional<int> version = document.FindInt("version");
  const base::ListValue* domains = document.FindList("domains");
  if (!version || !domains || domains->empty()) {
    LOG(ERROR) << "ru-trust: payload has no version or no domains; discarded";
    return;
  }
  if (*version <= local_state_->GetInteger(kRuTrustVersionPref)) {
    VLOG(1) << "ru-trust: served list is not newer; keeping ours";
    return;
  }

  local_state_->SetList(kRuTrustDomainsPref, domains->Clone());
  local_state_->SetInteger(kRuTrustVersionPref, *version);
  VLOG(1) << "ru-trust: stored " << domains->size() << " domains, version "
          << *version << "; applies on the next start";
}

}  // namespace growser_ru_trust
