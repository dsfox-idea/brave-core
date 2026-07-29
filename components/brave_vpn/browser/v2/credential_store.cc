/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/credential_store.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check_deref.h"
#include "base/json/values_util.h"
#include "base/types/to_address.h"
#include "base/values.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn::v2 {
namespace {

// Shared validity check for a single keyed credential in the slot dictionary:
// the credential string must be present and non-empty, and the (shared)
// expiration must be present and in the future.
bool IsCredentialValid(const base::DictValue& dict,
                       std::string_view credential_key) {
  if (dict.empty()) {
    return false;
  }

  const std::string* credential = dict.FindString(credential_key);
  if (!credential || credential->empty()) {
    return false;
  }

  const base::Value* expiration_value =
      dict.Find(kSubscriberCredentialExpirationKey);
  if (!expiration_value) {
    return false;
  }

  const std::optional<base::Time> expiration =
      base::ValueToTime(expiration_value);
  return expiration.has_value() && *expiration >= base::Time::Now();
}

// Reads the keyed credential and the shared expiration from the slot as one
// bundle, or nullopt if the credential is not valid. IsCredentialValid()
// verifies - on this same dict - that the credential string and a parseable,
// future expiration are both present, so the dereferences below cannot fail.
std::optional<CredentialStore::Credential> GetValidCredential(
    const base::DictValue& dict,
    std::string_view credential_key) {
  if (!IsCredentialValid(dict, credential_key)) {
    return std::nullopt;
  }
  return CredentialStore::Credential{
      .value = *dict.FindString(credential_key),
      .expiration =
          *base::ValueToTime(dict.Find(kSubscriberCredentialExpirationKey)),
  };
}

}  // namespace

CredentialStore::CredentialStore(PrefService* local_prefs)
    : local_prefs_(CHECK_DEREF(local_prefs)) {}

CredentialStore::~CredentialStore() = default;

bool CredentialStore::HasValidSubscriberCredential() const {
  return IsCredentialValid(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSubscriberCredentialKey);
}

std::optional<CredentialStore::Credential>
CredentialStore::GetValidSubscriberCredential() const {
  return GetValidCredential(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSubscriberCredentialKey);
}

void CredentialStore::SetSubscriberCredential(const Credential& credential) {
  base::DictValue dict;
  dict.Set(kSubscriberCredentialKey, credential.value);
  dict.Set(kSubscriberCredentialExpirationKey,
           base::TimeToValue(credential.expiration));
  local_prefs_->SetDict(prefs::kBraveVPNSubscriberCredential, std::move(dict));
}

bool CredentialStore::HasValidSkusCredential() const {
  return IsCredentialValid(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSkusCredentialKey);
}

std::optional<CredentialStore::Credential>
CredentialStore::GetValidSkusCredential() const {
  return GetValidCredential(
      local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential),
      kSkusCredentialKey);
}

void CredentialStore::SetSkusCredential(const Credential& credential) {
  base::DictValue dict;
  dict.Set(kSkusCredentialKey, credential.value);
  dict.Set(kSubscriberCredentialExpirationKey,
           base::TimeToValue(credential.expiration));
  local_prefs_->SetDict(prefs::kBraveVPNSubscriberCredential, std::move(dict));
  local_prefs_->SetTime(prefs::kBraveVPNLastCredentialExpiry,
                        credential.expiration);
}

bool CredentialStore::HasAnyCredential() const {
  return !local_prefs_->GetDict(prefs::kBraveVPNSubscriberCredential).empty();
}

void CredentialStore::Clear() {
  ::brave_vpn::ClearSubscriberCredential(base::to_address(local_prefs_));
}

}  // namespace brave_vpn::v2
