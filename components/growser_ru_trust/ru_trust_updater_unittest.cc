/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser (#95): the list served from our backend is only as good as the
// signature check that guards it. These fixtures were signed with the real
// release key, so a failure here means the key compiled into the browser and
// the key that signs the list have drifted apart - after which every browser
// discards every update in silence and keeps an ageing list.

#include "brave/components/growser_ru_trust/ru_trust_updater.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

namespace growser_ru_trust {

namespace {

// Signed with ../growser-keys/ru-trust.der, the key that signs the real list.
constexpr char kPayload[] =
    R"({"domains":["example.ru","sberbank.ru"],"generated":"2026-01-01","version":20260101})";

constexpr char kSignature[] =
    "MEUCIGVZGkVYjExLBH+CQRlDQ8ynWCXUqAw1UNm1hTfUGKyFAiEA317DSjVSbtdSL0TxggaPsAaPDnzZipyVD3gGJeRcTIA=";

}  // namespace

TEST(RuTrustUpdaterTest, AcceptsAPayloadSignedWithTheReleaseKey) {
  EXPECT_TRUE(RuTrustUpdater::VerifyForTesting(kPayload, kSignature));
}

TEST(RuTrustUpdaterTest, RejectsAnAddedDomain) {
  // The attack this signature exists to stop: one more domain, which would
  // let that CA vouch for one more site.
  std::string tampered(kPayload);
  const std::string opening = "[";
  tampered.replace(tampered.find(opening), opening.size(),
                   R"([\"evil.example\",)");
  EXPECT_FALSE(RuTrustUpdater::VerifyForTesting(tampered, kSignature));
}

TEST(RuTrustUpdaterTest, RejectsRubbishInsteadOfASignature) {
  EXPECT_FALSE(RuTrustUpdater::VerifyForTesting(kPayload, "not-base64"));
  EXPECT_FALSE(RuTrustUpdater::VerifyForTesting(kPayload, ""));
}

}  // namespace growser_ru_trust
