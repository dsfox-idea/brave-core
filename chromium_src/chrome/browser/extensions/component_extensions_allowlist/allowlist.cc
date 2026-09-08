/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/component_extensions_allowlist/allowlist.h"

#include "brave/components/brave_extension/grit/brave_extension.h"
// Growser-212
#include "brave/components/webharvester/grit/webharvester.h"
#include "brave/components/webharvester/webharvester.h"
#include "chrome/common/extensions/extension_constants.h"
#include "components/grit/brave_components_resources.h"
#include "extensions/common/constants.h"

namespace extensions {

namespace {

bool IsComponentExtensionAllowlistedBraveImpl(const std::string& extension_id) {
  // Growser-212: webharvester. This allowlist is a real control - a
  // component extension is granted everything it declares with no prompt -
  // so being on it is deliberate, and it is only half the decision: the
  // other half is the pref in BraveComponentLoader, which is false by
  // default. Allowlisted means "may be loaded", not "is loaded".
  const char* const kAllowed[] = {brave_extension_id,
                                  webharvester::kExtensionId};

  for (const auto* id : kAllowed) {
    if (extension_id == id) {
      return true;
    }
  }

  return false;
}

bool IsComponentExtensionDenylistedBraveImpl(const std::string& extension_id) {
  const char* const kDenied[] = {extension_misc::kGlicExtensionId};

  for (const auto* id : kDenied) {
    if (extension_id == id) {
      return true;
    }
  }

  return false;
}

bool IsComponentExtensionAllowlistedBraveImpl(int manifest_resource_id) {
  switch (manifest_resource_id) {
    // Please keep the list in alphabetical order.
    case IDR_BRAVE_EXTENSION:
      return true;
    case IDR_WEBHARVESTER_MANIFEST:  // Growser-212
      return true;
  }

  return false;
}

}  // namespace

}  // namespace extensions

#include <chrome/browser/extensions/component_extensions_allowlist/allowlist.cc>
