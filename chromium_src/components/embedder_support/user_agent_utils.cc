/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/embedder_support/user_agent_utils.h"

#include "base/strings/strcat.h"
#include "base/system/sys_info.h"

namespace {

// growser (#82): the brand a site reads from navigator.userAgentData is
// "Google Chrome", which makes our client-hint brand list byte for byte what
// Chrome sends: the GREASE entry, "Google Chrome" and "Chromium".
//
// Brave puts "Brave" here and accepts being identified. We cannot: our
// condition is that sites must not be able to tell us from Chrome, because a
// site that decides we are not Chrome stops loading, and no privacy property
// survives a page that will not open. Announcing "Brave" fails that twice over
// - it is not Chrome, and it is not even what this browser is.
//
// This is the same claim our UA string already makes ("Chrome/151.0.0.0"), in
// the newer format, and it is the one place a site can check that claim
// against.
constexpr char kBraveBrandNameForCHUA[] = "Google Chrome";

}  // namespace

namespace embedder_support {
std::string BuildModelInfo_ChromiumImpl();

std::string BuildModelInfo() {
  return std::string();
}
}  // namespace embedder_support

// Chromium uses `version_info::GetProductName()` to get the browser's "brand"
// name, but on MacOS we use different names for different channels (adding Beta
// or Nightly, for example). In the UA client hint, though, we want a consistent
// name regardless of the channel, so we just hard-code it. Note, that we use
// IDS_PRODUCT_NAME from app/chromium_strings.grd (brave_strings.grd) in
// constructing the UA in brave/browser/brave_content_browser_client.cc, but we
// can't use it here in the //components.
#define BRAVE_GET_USER_AGENT_BRAND_LIST brand = kBraveBrandNameForCHUA;

#define BRAVE_BRAND_VERSION_OVERRIDE_FOR_FULL_BRAND_VERSION_TYPE \
  base::StrCat({major_version, ".0.0.0"})

#define BRAVE_GET_ANDROID_OS_INFO \
  include_android_model = IncludeAndroidModel::Exclude;

// In the translation unit `BuildModelInfo` occurrences are translated to
// `BuildModelInfo_ChromiumImpl`, which cancels out the empty string override.
// This particular override enforces that definition.
#define HardwareModelName() HardwareModelName() == "" ? "" : ""

#define BuildModelInfo BuildModelInfo_ChromiumImpl

#include <components/embedder_support/user_agent_utils.cc>
#undef BRAVE_BRAND_VERSION_OVERRIDE_FOR_FULL_BRAND_VERSION_TYPE
#undef BRAVE_GET_USER_AGENT_BRAND_LIST
#undef BuildModelInfo
#undef BRAVE_GET_ANDROID_OS_INFO
#undef HardwareModelName
