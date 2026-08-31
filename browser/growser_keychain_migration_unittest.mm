/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// Tests for the one-time keychain migration from the pre-rebrand item name
// ("Brave Safe Storage") to ours ("Growser Safe Storage"). The test names
// are unique to this file and never the real ones, so running it cannot
// migrate - or damage - any real item, including on a developer machine.

#include <Security/Security.h>

#include <string>

#include "base/strings/sys_string_conversions.h"
#include "components/os_crypt/common/growser_keychain_migration.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kLegacyService[] = "Growser Unittest Migration Legacy Service";
constexpr char kLegacyAccount[] = "Growser Unittest Migration Legacy Account";
constexpr char kNewService[] = "Growser Unittest Migration New Service";
constexpr char kNewAccount[] = "Growser Unittest Migration New Account";
constexpr char kValue[] = "unit test key material";

OSStatus DeleteItem(const std::string& service, const std::string& account) {
  CFMutableDictionaryRef query = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  CFDictionaryAddValue(query, kSecClass, kSecClassGenericPassword);
  base::apple::ScopedCFTypeRef<CFStringRef> service_ref(
      base::SysCFStringRef(service));
  base::apple::ScopedCFTypeRef<CFStringRef> account_ref(
      base::SysCFStringRef(account));
  CFDictionaryAddValue(query, kSecAttrService, service_ref.get());
  CFDictionaryAddValue(query, kSecAttrAccount, account_ref.get());
  OSStatus status = SecItemDelete(query);
  CFRelease(query);
  return status;
}

struct GrowserKeychainMigrationTest : testing::Test {
  ~GrowserKeychainMigrationTest() override {
    // The fixtures this test creates are its own; remove them either way.
    DeleteItem(kLegacyService, kLegacyAccount);
    DeleteItem(kNewService, kNewAccount);
  }
};

}  // namespace

TEST_F(GrowserKeychainMigrationTest, CopiesLegacyValueUnderNewName) {
  ASSERT_TRUE(growser::os_crypt::AddKeychainItemWithValue(
      kLegacyService, kLegacyAccount, kValue));

  EXPECT_TRUE(growser::os_crypt::MigrateKeychainItemForTest(
      kLegacyService, kLegacyAccount, kNewService, kNewAccount));
  EXPECT_TRUE(growser::os_crypt::HasKeychainItem(kNewService, kNewAccount));

  auto value = growser::os_crypt::FindKeychainItemValue(kNewService,
                                                        kNewAccount);
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, kValue);
}

TEST_F(GrowserKeychainMigrationTest, NoLegacyItemMeansNoNewItem) {
  EXPECT_FALSE(growser::os_crypt::HasKeychainItem(kNewService, kNewAccount));
  EXPECT_FALSE(growser::os_crypt::MigrateKeychainItemForTest(
      kLegacyService, kLegacyAccount, kNewService, kNewAccount));
  EXPECT_FALSE(growser::os_crypt::HasKeychainItem(kNewService, kNewAccount));
}

TEST_F(GrowserKeychainMigrationTest, ExistingNewItemIsLeftAlone) {
  ASSERT_TRUE(growser::os_crypt::AddKeychainItemWithValue(
      kNewService, kNewAccount, "fresh"));
  ASSERT_TRUE(growser::os_crypt::AddKeychainItemWithValue(
      kLegacyService, kLegacyAccount, "stale"));

  EXPECT_FALSE(growser::os_crypt::MigrateKeychainItemForTest(
      kLegacyService, kLegacyAccount, kNewService, kNewAccount));

  auto value = growser::os_crypt::FindKeychainItemValue(kNewService,
                                                        kNewAccount);
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, "fresh");
}