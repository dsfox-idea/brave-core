/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef COMPONENTS_OS_CRYPT_COMMON_GROWSER_KEYCHAIN_MIGRATION_H_
#define COMPONENTS_OS_CRYPT_COMMON_GROWSER_KEYCHAIN_MIGRATION_H_

#include <optional>
#include <string>

namespace growser::os_crypt {

// True when the login keychain holds a generic-password item with the given
// service and account names. Never prompts: an existence probe reads no data.
bool HasKeychainItem(const std::string& service, const std::string& account);

// Returns the item's value, or nullopt when absent or the read is refused.
// Reading an item created by another code signature is what makes macOS ask
// for the login password with an "Always Allow" choice.
std::optional<std::string> FindKeychainItemValue(const std::string& service,
                                                 const std::string& account);

// Creates a generic-password item holding `value`. Returns false on error
// (including errSecDuplicateItem, which callers treat as already-migrated).
bool AddKeychainItemWithValue(const std::string& service,
                              const std::string& account,
                              const std::string& value);

// One-time migration from the pre-rebrand keychain item. If the new-named
// item does not exist yet and the legacy one does, copies the legacy value
// under the new name, so every secret encrypted with the old key - saved
// passwords, and cookies from the builds before encryption became opt-in -
// keeps decrypting. Idempotent; every failure falls back to the upstream
// behavior of minting a fresh random key. Runs once per process.
void MigrateLegacyBraveKeychainItem();

// Test seam: the same migration against caller-chosen names, so the unit
// test never touches the real "Growser Safe Storage" / "Brave Safe Storage"
// items. Returns true when the value was copied.
bool MigrateKeychainItemForTest(const std::string& legacy_service,
                                const std::string& legacy_account,
                                const std::string& service,
                                const std::string& account);

}  // namespace growser::os_crypt

#endif  // COMPONENTS_OS_CRYPT_COMMON_GROWSER_KEYCHAIN_MIGRATION_H_