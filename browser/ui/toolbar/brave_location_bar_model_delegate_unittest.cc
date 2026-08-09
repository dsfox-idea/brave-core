/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/constants/url_constants.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

// The UI scheme carries the product name, and growser renamed it. Building the
// expectation from kBraveUIScheme keeps the test about the rewrite of chrome://
// rather than about what the browser happens to be called.
using BraveLocationBarModelDelegateTest = testing::Test;

TEST_F(BraveLocationBarModelDelegateTest, ResolvesChromeSchemeToBrave) {
  GURL url("chrome://sync/");
  std::u16string formatted_url = base::UTF8ToUTF16(url.spec());
  BraveLocationBarModelDelegate::FormattedStringFromURL(url, &formatted_url);
  ASSERT_STREQ(base::UTF16ToASCII(formatted_url).c_str(),
               base::StrCat({kBraveUIScheme, "://sync/"}).c_str());
}
