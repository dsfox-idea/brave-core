/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/os_crypt/common/keychain_password_mac.h"

#include <Security/Security.h>

#include <optional>
#include <string>
#include <utility>

#include "base/apple/foundation_util.h"
#include "base/apple/osstatus_logging.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "components/os_crypt/common/growser_keychain_migration.h"

namespace {

KeychainPassword::KeychainNameType& GetBraveServiceName();
KeychainPassword::KeychainNameType& GetBraveAccountName();

}  // namespace

namespace growser::os_crypt {

namespace {

// Raw Sec query helpers. KeychainV2 (used by the included base file) does not
// expose "add this exact value", which is what migrating the pre-rebrand key
// needs, so these go straight to the SecItem API.

CFMutableDictionaryRef BaseItemQuery(const std::string& service,
                                     const std::string& account) {
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
  return query;
}

}  // namespace

bool HasKeychainItem(const std::string& service, const std::string& account) {
  base::apple::ScopedCFTypeRef<CFMutableDictionaryRef> query(
      BaseItemQuery(service, account));
  CFDictionaryAddValue(query.get(), kSecMatchLimit, kSecMatchLimitOne);
  base::apple::ScopedCFTypeRef<CFTypeRef> result;
  OSStatus status = SecItemCopyMatching(query.get(), result.InitializeInto());
  return status == errSecSuccess;
}

std::optional<std::string> FindKeychainItemValue(
    const std::string& service, const std::string& account) {
  base::apple::ScopedCFTypeRef<CFMutableDictionaryRef> query(
      BaseItemQuery(service, account));
  CFDictionaryAddValue(query.get(), kSecReturnData, kCFBooleanTrue);
  CFDictionaryAddValue(query.get(), kSecMatchLimit, kSecMatchLimitOne);
  // SecItemCopyMatching hands the value back as a CFTypeRef; the idiom
  // (crypto/apple/keychain_v2.mm) is to receive it that way and cast.
  base::apple::ScopedCFTypeRef<CFTypeRef> result;
  OSStatus status = SecItemCopyMatching(query.get(), result.InitializeInto());
  if (status != errSecSuccess) {
    return std::nullopt;
  }
  base::apple::ScopedCFTypeRef<CFDataRef> data(
      base::apple::CFCast<CFDataRef>(result.get()));
  if (!data) {
    return std::nullopt;
  }
  return std::string(
      reinterpret_cast<const char*>(CFDataGetBytePtr(data.get())),
      CFDataGetLength(data.get()));
}

bool AddKeychainItemWithValue(const std::string& service,
                              const std::string& account,
                              const std::string& value) {
  base::apple::ScopedCFTypeRef<CFMutableDictionaryRef> item(
      BaseItemQuery(service, account));
  base::apple::ScopedCFTypeRef<CFDataRef> value_ref(CFDataCreate(
      kCFAllocatorDefault,
      reinterpret_cast<const UInt8*>(value.data()), value.size()));
  CFDictionaryAddValue(item.get(), kSecValueData, value_ref.get());
  OSStatus status = SecItemAdd(item.get(), nullptr);
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "growser keychain migration add failed";
  }
  return status == errSecSuccess;
}

bool MigrateKeychainItemForTest(const std::string& legacy_service,
                                const std::string& legacy_account,
                                const std::string& service,
                                const std::string& account) {
  if (HasKeychainItem(service, account)) {
    return false;
  }
  auto legacy = FindKeychainItemValue(legacy_service, legacy_account);
  if (!legacy) {
    return false;
  }
  return AddKeychainItemWithValue(service, account, *legacy);
}

void MigrateLegacyBraveKeychainItem() {
  // C++11 magic statics: exactly once per process, lazily, thread-safe.
  static const bool migrated = [] {
    // The pre-rebrand item names. The migration read of the legacy item is
    // the ONE macOS access prompt a user may see (the item was created under
    // a Brave code signature); after it no Brave identity remains anywhere
    // in the keychain. Every failure simply leaves the base behavior in
    // place: it mints a fresh random key under the Growser name.
    MigrateKeychainItemForTest("Brave Safe Storage", "Brave",
                               "Growser Safe Storage", "Growser");
    return true;
  }();
  (void)migrated;
}

}  // namespace growser::os_crypt

#define BRAVE_GET_SERVICE_NAME return GetBraveServiceName();
#define BRAVE_GET_ACCOUNT_NAME return GetBraveAccountName();
#include <components/os_crypt/common/keychain_password_mac.mm>
#undef BRAVE_GET_SERVICE_NAME
#undef BRAVE_GET_ACCOUNT_NAME

namespace {

KeychainPassword::KeychainNameType& GetBraveServiceName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-edge")) {
    static KeychainNameContainerType kEdgeServiceName(
        "Microsoft Edge Safe Storage");
    return *kEdgeServiceName;
  } else if (command_line->HasSwitch("import-yandex")) {
    static KeychainNameContainerType kYandexServiceName("Yandex Safe Storage");
    return *kYandexServiceName;
  } else if (command_line->HasSwitch("import-whale")) {
    static KeychainNameContainerType kWhaleServiceName("Whale Safe Storage");
    return *kWhaleServiceName;
  } else if (command_line->HasSwitch("import-chrome")) {
    static KeychainNameContainerType kChromeServiceName("Chrome Safe Storage");
    return *kChromeServiceName;
  } else if (command_line->HasSwitch("import-vivaldi")) {
    static KeychainNameContainerType kVivaldiServiceName(
        "Vivaldi Safe Storage");
    return *kVivaldiServiceName;
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    static KeychainNameContainerType kChromiumServiceName(
        "Chromium Safe Storage");
    return *kChromiumServiceName;
  } else if (command_line->HasSwitch("import-opera")) {
    static KeychainNameContainerType kOperaServiceName("Opera Safe Storage");
    return *kOperaServiceName;
  } else {
    // growser: our own keychain item, created by our code signature, so the
    // first access never has to ask the user to let a Brave-signed item go.
    // The pre-rebrand item migrates into this name once (see
    // MigrateLegacyBraveKeychainItem), keeping every value encrypted with
    // the old key readable.
    growser::os_crypt::MigrateLegacyBraveKeychainItem();
    static KeychainNameContainerType kGrowserDefaultServiceName(
        "Growser Safe Storage");
    return *kGrowserDefaultServiceName;
  }
}

KeychainPassword::KeychainNameType& GetBraveAccountName() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("import-edge")) {
    static KeychainNameContainerType kEdgeAccountName("Microsoft Edge");
    return *kEdgeAccountName;
  } else if (command_line->HasSwitch("import-yandex")) {
    static KeychainNameContainerType kYandexAccountName("Yandex");
    return *kYandexAccountName;
  } else if (command_line->HasSwitch("import-whale")) {
    static KeychainNameContainerType kWhaleAccountName("Whale");
    return *kWhaleAccountName;
  } else if (command_line->HasSwitch("import-chrome")) {
    static KeychainNameContainerType kChromeAccountName("Chrome");
    return *kChromeAccountName;
  } else if (command_line->HasSwitch("import-vivaldi")) {
    static KeychainNameContainerType kVivaldiAccountName("Vivaldi");
    return *kVivaldiAccountName;
  } else if (command_line->HasSwitch("import-chromium") ||
             command_line->HasSwitch("import-brave")) {
    static KeychainNameContainerType kChromiumAccountName("Chromium");
    return *kChromiumAccountName;
  } else if (command_line->HasSwitch("import-opera")) {
    static KeychainNameContainerType kOperaAccountName("Opera");
    return *kOperaAccountName;
  } else {
    static KeychainNameContainerType kGrowserDefaultAccountName("Growser");
    return *kGrowserDefaultAccountName;
  }
}

}  // namespace
