/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/growser_ru_trust/ru_trust_anchors.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace growser_ru_trust {

namespace {

// SHA-256 fingerprint of "Russian Trusted Root CA".
constexpr char kExpectedRootSha256[] =
    "D26D2D0231B7C39F92CC738512BA54103519E4405D68B5BD703E9788CA8ECF31";

const base::DictValue& GetOnlyAnchor(const base::ListValue& anchors) {
  return anchors.front().GetDict();
}

std::vector<std::string> GetPermittedDnsNames(const base::ListValue& anchors) {
  std::vector<std::string> names;
  const base::ListValue* list = GetOnlyAnchor(anchors)
                                      .FindDict("constraints")
                                      ->FindList("permitted_dns_names");
  for (const base::Value& name : *list) {
    names.push_back(name.GetString());
  }
  return names;
}

}  // namespace

// The default has the shape ProfileNetworkContextService expects when it
// reads kCACertificatesWithConstraints: exactly one anchor, with a
// certificate and a non-empty set of constraints.
TEST(GrowserRuTrustAnchorsTest, PrefDefaultHasExpectedShape) {
  base::ListValue anchors = GetTrustAnchorsPrefDefault();
  ASSERT_EQ(1u, anchors.size());

  const base::DictValue& anchor = GetOnlyAnchor(anchors);
  EXPECT_NE(nullptr, anchor.FindString("certificate"));

  const base::DictValue* constraints = anchor.FindDict("constraints");
  ASSERT_NE(nullptr, constraints);
  const base::ListValue* dns_names =
      constraints->FindList("permitted_dns_names");
  ASSERT_NE(nullptr, dns_names);
  EXPECT_FALSE(dns_names->empty());
}

// The bundled certificate really is that root, and parses as valid X.509.
TEST(GrowserRuTrustAnchorsTest, CertificateIsRussianTrustedRootCa) {
  base::ListValue anchors = GetTrustAnchorsPrefDefault();
  const std::string* cert_b64 = GetOnlyAnchor(anchors).FindString("certificate");
  ASSERT_NE(nullptr, cert_b64);

  std::optional<std::vector<uint8_t>> der = base::Base64Decode(*cert_b64);
  ASSERT_TRUE(der.has_value());

  scoped_refptr<net::X509Certificate> cert =
      net::X509Certificate::CreateFromBytes(*der);
  ASSERT_TRUE(cert);
  EXPECT_EQ("Russian Trusted Root CA", cert->subject().common_name);
  EXPECT_EQ(kExpectedRootSha256,
            base::HexEncode(cert->CalculateChainFingerprint256()));
}

// Every constraint passes the validation ProfileNetworkContextService does
// (IsValidDNSConstraint: ASCII, no longer than 255 characters). One that
// does not would make it drop the whole anchor, silently.
TEST(GrowserRuTrustAnchorsTest, AllDnsConstraintsAreValid) {
  for (const std::string& name : GetPermittedDnsNames(GetTrustAnchorsPrefDefault())) {
    EXPECT_TRUE(base::IsStringASCII(name)) << name;
    EXPECT_LE(name.length(), 255u) << name;
    EXPECT_FALSE(name.empty());
    // A wildcard or a leading dot in permittedSubtrees would widen the trust
    // beyond what is intended.
    EXPECT_EQ(std::string::npos, name.find('*')) << name;
    EXPECT_NE('.', name.front()) << name;
  }
}

// The point of the exercise: the banks that moved to this root are covered.
TEST(GrowserRuTrustAnchorsTest, CoversKnownRussianBankDomains) {
  std::vector<std::string> names =
      GetPermittedDnsNames(GetTrustAnchorsPrefDefault());
  for (std::string_view expected :
       {"sberbank.ru", "vtb.ru", "alfabank.ru", "gosuslugi.ru", "mkb.ru",
        "gazprombank.ru", "tinkoff.ru", "nalog.ru"}) {
    EXPECT_TRUE(std::ranges::contains(names, expected)) << expected;
  }
}

// The invariant that matters: this root must never be able to vouch for the
// global services. The test is here to catch a careless edit of the domain
// list, which is the way that would happen.
TEST(GrowserRuTrustAnchorsTest, DoesNotCoverGlobalServices) {
  std::vector<std::string> names =
      GetPermittedDnsNames(GetTrustAnchorsPrefDefault());
  for (std::string_view forbidden :
       {"google.com", "gmail.com", "youtube.com", "apple.com", "icloud.com",
        "microsoft.com", "github.com", "cloudflare.com", "mozilla.org",
        "facebook.com", "telegram.org", "whatsapp.com", "amazon.com",
        "brave.com", "com", "ru", "org", "net"}) {
    EXPECT_FALSE(std::ranges::contains(names, forbidden)) << forbidden;
  }
}

}  // namespace growser_ru_trust
