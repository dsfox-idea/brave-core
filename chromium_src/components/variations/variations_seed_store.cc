/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/variations/variations_seed_store.h"

#include <ranges>

#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/no_destructor.h"
#include "crypto/sign.h"

namespace variations {

// A non-anonymous class to friend with base::CurrentTestVendor.
class PublicKeyWrapper {
 public:
  static const crypto::keypair::PublicKey& GetPublicKey(
      const crypto::keypair::PublicKey& public_key);
};

}  // namespace variations

#define Verify(signature_algorithm, public_key, seed_bytes, signature)    \
  Verify(signature_algorithm, PublicKeyWrapper::GetPublicKey(public_key), \
         seed_bytes, signature)

#include <components/variations/variations_seed_store.cc>

#undef Verify

namespace variations {

void VariationsSeedStore::SetSessionCountry(std::string_view country_code) {
  seed_reader_writer_->SetSessionCountry(country_code);
}

// static
const crypto::keypair::PublicKey& PublicKeyWrapper::GetPublicKey(
    const crypto::keypair::PublicKey& public_key) {
  // Only kPublicKey should be passed here. This is a sanity check.
  DCHECK(std::ranges::equal(public_key.ToSubjectPublicKeyInfo(), kPublicKey));

  // If we are in a Chromium test, return the original public key to let those
  // tests check everything they need.
  if (base::CurrentTestVendor::Get() == base::TestVendor::kChromium) {
    return public_key;
  }

  // The growser seed key: an ECDSA P-256 SubjectPublicKeyInfo, the algorithm
  // the caller verifies with (crypto::sign::SignatureKind::ECDSA_SHA256).
  // Its private half is seed.der, outside this repository - see growser
  // docs/signing.md. The identifiers keep Brave's spelling to keep the diff
  // against upstream's override minimal across Chromium bumps.
  static constexpr uint8_t kBravePublicKeyBytes[] = {
      0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02,
      0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
      0x42, 0x00, 0x04, 0xbd, 0xbb, 0x7e, 0x6b, 0xe3, 0x86, 0x64, 0x01, 0x8e,
      0x5c, 0x41, 0x5d, 0x4b, 0x34, 0xac, 0xa7, 0xa9, 0x3c, 0x0e, 0xf5, 0x89,
      0x66, 0x59, 0x67, 0x39, 0xbc, 0x33, 0xc0, 0x33, 0xae, 0xab, 0x23, 0xdc,
      0x89, 0x57, 0x4d, 0xda, 0x76, 0x91, 0x79, 0x4d, 0xb5, 0x77, 0x2f, 0x7c,
      0x1a, 0x89, 0xee, 0xa5, 0x63, 0x10, 0x56, 0xef, 0x3e, 0x7b, 0xdb, 0x03,
      0xd7, 0x48, 0x9b, 0xfc, 0x67, 0xa1, 0xce,
  };

  static const base::NoDestructor<crypto::keypair::PublicKey> kBravePublicKey(
      [] {
        auto brave_public_key =
            crypto::keypair::PublicKey::FromSubjectPublicKeyInfo(
                kBravePublicKeyBytes);
        DCHECK(brave_public_key);
        return std::move(*brave_public_key);
      }());

  return *kBravePublicKey;
}

}  // namespace variations

#undef BRAVE_K_PUBLIC_KEY
