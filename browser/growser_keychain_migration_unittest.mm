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

#include "base/process/process_handle.h"
#include "base/strings/stringprintf.h"
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
      base::SysUTF8ToCFStringRef(service));
  base::apple::ScopedCFTypeRef<CFStringRef> account_ref(
      base::SysUTF8ToCFStringRef(account));
  CFDictionaryAddValue(query, kSecAttrService, service_ref.get());
  CFDictionaryAddValue(query, kSecAttrAccount, account_ref.get());
  OSStatus status = SecItemDelete(query);
  CFRelease(query);
  return status;
}

// Every test gets item names of its own. These run against the REAL login
// keychain and the launcher runs tests in parallel processes, so names shared
// across the suite raced: one test created the new-named item while a sibling
// asserted it was absent, and the add came back errSecDuplicateItem. Three
// builds in a row carried a red test here that the launcher's retry turned
// green again - a gate that has stopped measuring (lessons 44/47). The test
// name separates siblings; the pid separates parallel processes, a retry, and
// anything an interrupted earlier run left behind.
struct GrowserKeychainMigrationTest : testing::Test {
  GrowserKeychainMigrationTest() {
    const std::string unique = base::StringPrintf(
        " %s.%d",
        testing::UnitTest::GetInstance()->current_test_info()->name(),
        base::GetCurrentProcId());
    legacy_service_ = kLegacyService + unique;
    legacy_account_ = kLegacyAccount + unique;
    new_service_ = kNewService + unique;
    new_account_ = kNewAccount + unique;
  }

  ~GrowserKeychainMigrationTest() override {
    // The items this test creates are its own; remove them either way.
    DeleteItem(legacy_service_, legacy_account_);
    DeleteItem(new_service_, new_account_);
  }

  std::string legacy_service_;
  std::string legacy_account_;
  std::string new_service_;
  std::string new_account_;
};

}  // namespace

TEST_F(GrowserKeychainMigrationTest, CopiesLegacyValueUnderNewName) {
  ASSERT_TRUE(growser::os_crypt::AddKeychainItemWithValue(
      legacy_service_, legacy_account_, kValue));

  EXPECT_TRUE(growser::os_crypt::MigrateKeychainItemForTest(
      legacy_service_, legacy_account_, new_service_, new_account_));
  EXPECT_TRUE(growser::os_crypt::HasKeychainItem(new_service_, new_account_));

  auto value = growser::os_crypt::FindKeychainItemValue(new_service_,
                                                        new_account_);
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, kValue);
}

TEST_F(GrowserKeychainMigrationTest, NoLegacyItemMeansNoNewItem) {
  EXPECT_FALSE(growser::os_crypt::HasKeychainItem(new_service_, new_account_));
  EXPECT_FALSE(growser::os_crypt::MigrateKeychainItemForTest(
      legacy_service_, legacy_account_, new_service_, new_account_));
  EXPECT_FALSE(growser::os_crypt::HasKeychainItem(new_service_, new_account_));
}

TEST_F(GrowserKeychainMigrationTest, ExistingNewItemIsLeftAlone) {
  ASSERT_TRUE(growser::os_crypt::AddKeychainItemWithValue(
      new_service_, new_account_, "fresh"));
  ASSERT_TRUE(growser::os_crypt::AddKeychainItemWithValue(
      legacy_service_, legacy_account_, "stale"));

  EXPECT_FALSE(growser::os_crypt::MigrateKeychainItemForTest(
      legacy_service_, legacy_account_, new_service_, new_account_));

  auto value = growser::os_crypt::FindKeychainItemValue(new_service_,
                                                        new_account_);
  ASSERT_TRUE(value);
  EXPECT_EQ(*value, "fresh");
}