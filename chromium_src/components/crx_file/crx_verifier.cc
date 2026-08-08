/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/crx_file/crx_verifier.h"

#include <array>
#include <utility>

#include "base/containers/span.h"

namespace {

// The growser publisher key, accepted in addition to upstream's
// kPublisherKeyHash. It verifies updates of the browser itself and any
// component we publish ourselves; upstream's key stays accepted because the
// components we proxy are still Google's.
//
// The value is the SHA-256 of the SubjectPublicKeyInfo, which is what
// VerifyCrx3() hashes and compares. The private half is publisher.der (PKCS #8
// DER, the format crx_build_action expects) and lives outside this repository -
// see growser docs/signing.md.
//
// The identifiers keep Brave's spelling on purpose: IsBravePublisher() is
// called from patches/components-crx_file-crx_verifier.cc.patch, so renaming
// them here would mean re-cutting that patch on every Chromium bump for no
// gain.
constexpr uint8_t kBravePublisherKeyHash[] = {
    0xe6, 0x04, 0x72, 0x47, 0xa5, 0x13, 0x67, 0xef, 0x44, 0x3f, 0xe1,
    0x92, 0x5a, 0xad, 0x30, 0x3c, 0x47, 0xe4, 0xc2, 0x91, 0x2d, 0x5a,
    0x22, 0x4e, 0x61, 0x77, 0x29, 0x3f, 0x4d, 0xb7, 0x37, 0x70};

auto GetBravePublisherKeyHash() {
  static auto brave_publisher_key = std::to_array(kBravePublisherKeyHash);
  return base::span(brave_publisher_key);
}

// Used in the patch in crx_verifier.cc.
bool IsBravePublisher(base::span<const uint8_t> key_hash) {
  return GetBravePublisherKeyHash() == key_hash;
}

}  // namespace

namespace crx_file {

void SetBravePublisherKeyHashForTesting(base::span<const uint8_t> test_key) {
  GetBravePublisherKeyHash().copy_from(test_key);
}

}  // namespace crx_file

#include <components/crx_file/crx_verifier.cc>
