/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/install_static/install_modes.h"

#include "chrome/install_static/buildflags.h"

namespace install_static {

namespace {

// growser (#128): one path for every build kind, official included. The
// BraveSoftware\Update\{Clients,ClientState,ClientStateMedium} vendor keys
// belonged to Brave's Omaha, which this product does not run (MS Store
// updates, growser#76); and every existing install already keeps its state -
// including the crash-consent usagestats value - under Software\Growser.
// An official build moving it to a Brave vendor key would both orphan that
// state and write another product's registry identity.
std::wstring GetUnregisteredKeyPathForProduct() {
  return std::wstring(L"Software\\").append(kProductPathName);
}

}  // namespace

std::wstring GetClientsKeyPath(const wchar_t* app_guid) {
  return GetUnregisteredKeyPathForProduct();
}

std::wstring GetClientStateKeyPath(const wchar_t* app_guid) {
  return GetUnregisteredKeyPathForProduct();
}

std::wstring GetClientStateMediumKeyPath(const wchar_t* app_guid) {
  return GetUnregisteredKeyPathForProduct();
}

}  // namespace install_static
