// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "brave/components/safebrowsing/buildflags.h"
#include "brave/components/static_redirect_helper/static_redirect_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave {

// Growser-45: the favicon and autofill redirects go to OUR backend, not to
// Brave's proxy. Chromium's UI asks Google's faviconV2 service for the icons
// of sites in history, bookmarks and new-tab tiles, so whoever answers it
// learns which sites a person keeps; sending that to Brave instead of Google
// only moves the knowledge. The expectations below are built from the same
// buildflag the code redirects to, so moving the endpoint moves the test
// with it rather than reddening it.
class StaticRedirectHelperUnitTest : public testing::Test {
 public:
  StaticRedirectHelperUnitTest() = default;
  ~StaticRedirectHelperUnitTest() override = default;
};

TEST_F(StaticRedirectHelperUnitTest, FaviconServiceMatch) {
  GURL old_url = GURL(
      "https://t0.gstatic.com/"
      "faviconV2?client=chrome&nfrp=2&check_seen=true&size=32&min_size=16&max_"
      "size=256&fallback_opts=TYPE,SIZE,URL&url=https://search.brave.com/");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(
      new_url.spec(),
      std::string("https://") + BUILDFLAG(BRAVE_REDIRECTOR_ENDPOINT)
          + "/faviconV2?client=chrome&nfrp=2&check_seen=true&size=32&min_size"
            "=16&max_size=256&fallback_opts=TYPE,SIZE,URL"
            "&url=https://search.brave.com/");
}

TEST_F(StaticRedirectHelperUnitTest, FaviconServiceMatchNoParams) {
  GURL old_url = GURL("https://t0.gstatic.com/faviconV2");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), std::string("https://")
                                + BUILDFLAG(BRAVE_REDIRECTOR_ENDPOINT)
                                + "/faviconV2");
}

TEST_F(StaticRedirectHelperUnitTest, DontMatchGstaticImages) {
  GURL old_url = GURL(
      "https://t0.gstatic.com/"
      "images?client=chrome&nfrp=2&check_seen=true&size=32&min_size=16&max_"
      "size=256&fallback_opts=TYPE,SIZE,URL&url=https://search.brave.com/");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "");
}

TEST_F(StaticRedirectHelperUnitTest, FaviconServicePartialMatch) {
  GURL old_url = GURL("https://t0.gstatic.com/faviconV");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "");
}

TEST_F(StaticRedirectHelperUnitTest, FaviconServiceCloseButNoMatch) {
  GURL old_url = GURL("https://t0.gstatic.com/faviconV1");
  GURL new_url;

  brave::StaticRedirectHelper(old_url, &new_url);

  EXPECT_EQ(new_url.spec(), "");
}

}  // namespace brave
