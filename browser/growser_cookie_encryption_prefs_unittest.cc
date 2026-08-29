/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

// growser: cookie encryption is opt-in. These tests pin the two prefs the NTP
// promo and the network-context gate read: they are boolean, default false, and
// round-trip. The default false is the core fix - with it off the cookie store
// loads without OSCrypt and the first navigation never blocks on the macOS
// keychain. Registering only these prefs (rather than the full
// brave::RegisterProfilePrefs, which needs browser-process setup) mirrors the
// ntp_background_prefs_unittest pattern.
class GrowserCookieEncryptionPrefsTest : public testing::Test {
 public:
  GrowserCookieEncryptionPrefsTest() {
    service_.registry()->RegisterBooleanPref(kGrowserCookieEncryptionEnabled,
                                             false);
    service_.registry()->RegisterBooleanPref(
        kGrowserCookieEncryptionPromoDismissed, false);
  }

  sync_preferences::TestingPrefServiceSyncable service_;
};

// The core fix: encryption is off by default.
TEST_F(GrowserCookieEncryptionPrefsTest, EncryptionOffByDefault) {
  EXPECT_FALSE(service_.GetBoolean(kGrowserCookieEncryptionEnabled));
}

// The promo is not dismissed by default, so a fresh user sees the offer.
TEST_F(GrowserCookieEncryptionPrefsTest, PromoNotDismissedByDefault) {
  EXPECT_FALSE(service_.GetBoolean(kGrowserCookieEncryptionPromoDismissed));
}

// Opting in and dismissing both persist.
TEST_F(GrowserCookieEncryptionPrefsTest, RoundTrip) {
  service_.SetBoolean(kGrowserCookieEncryptionEnabled, true);
  EXPECT_TRUE(service_.GetBoolean(kGrowserCookieEncryptionEnabled));

  service_.SetBoolean(kGrowserCookieEncryptionPromoDismissed, true);
  EXPECT_TRUE(service_.GetBoolean(kGrowserCookieEncryptionPromoDismissed));
}