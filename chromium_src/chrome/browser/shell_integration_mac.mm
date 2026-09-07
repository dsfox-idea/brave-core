/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/shell_integration.h"

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "build/branding_buildflags.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"

// All above headers copied from original shell_integration_mac.mm are
// included to prevent below GOOGLE_CHROME_BUILD affect them.

#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)

// Growser-208: GetDefaultBrowser and IsDefaultHandlerForUTType are no longer
// replaced here. They were, only to swap upstream's IsAnotherChromeChannel for
// a test that could tell Brave from Brave Origin - those two share the
// three-component bundle id prefix "com.brave.Browser", so upstream's test
// calls each one another channel of the other. We ship no Origin, so that
// problem does not exist for us, and upstream's version is the better code
// anyway: it lops OUR OWN bundle id to three components and compares,
// hardcoding no identity at all. com.growser.Browser against
// com.growser.Browser.beta matches; against com.brave.Browser it does not.
//
// The replacement had gone stale the way a hardcoded identity always does - it
// tested for "com.brave.Browser", which our bundle id has never been, so
// OTHER_MODE_IS_DEFAULT could never be reported. Deleting it is the fix.
//
// The define above stays: it is what compiles upstream's same-brand path,
// which sits behind #if BUILDFLAG(GOOGLE_CHROME_BRANDING).
#define GetPlatformSpecificDefaultWebClientSetPermission \
  GetPlatformSpecificDefaultWebClientSetPermission_Unused
#include <chrome/browser/shell_integration_mac.mm>
#undef GetPlatformSpecificDefaultWebClientSetPermission
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING

namespace shell_integration {

namespace internal {

DefaultWebClientSetPermission GetPlatformSpecificDefaultWebClientSetPermission(
    WebClientSetMethod method) {
  return SET_DEFAULT_UNATTENDED;
}

}  // namespace internal

}  // namespace shell_integration
