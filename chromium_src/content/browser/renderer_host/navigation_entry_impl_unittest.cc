/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>

#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/constants/url_constants.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/referrer.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace content {

namespace {
// The UI scheme carries the product name, and growser renamed it. Building the
// expectation from kBraveUIScheme keeps the test about the rewrite of chrome://
// rather than about what the browser happens to be called.
std::u16string UIUrl(std::string_view path) {
  return base::UTF8ToUTF16(base::StrCat({kBraveUIScheme, "://", path}));
}
}  // namespace

class BraveNavigationEntryTest : public testing::Test {
 private:
  BrowserTaskEnvironment task_environment_;
  TestBrowserContext browser_context_;

 protected:
  std::unique_ptr<NavigationEntry> CreateEntry(const GURL& url) {
    return NavigationController::CreateNavigationEntry(
        url, Referrer(), /* initiator_origin= */ std::nullopt,
        /* initiator_base_url= */ std::nullopt, ui::PAGE_TRANSITION_TYPED,
        /* is_renderer_initiated= */ false, /* extra_headers= */ std::string(),
        &browser_context_, /* blob_url_loader_factory= */ nullptr);
  }
};

TEST_F(BraveNavigationEntryTest,
       GetTitleForDisplayConvertsChromeSchemeToBrave) {
  auto entry = CreateEntry(GURL("chrome://settings"));
  EXPECT_EQ(UIUrl("settings"), entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("chrome://history"));
  EXPECT_EQ(UIUrl("history"), entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("chrome://flags"));
  EXPECT_EQ(UIUrl("flags"), entry->GetTitleForDisplay());
}

TEST_F(BraveNavigationEntryTest, GetTitleForDisplayPreservesExplicitTitle) {
  auto entry = CreateEntry(GURL("chrome://settings"));
  entry->SetTitle(u"Settings");
  EXPECT_EQ(u"Settings", entry->GetTitleForDisplay());
}

TEST_F(BraveNavigationEntryTest,
       GetTitleForDisplayDoesNotAffectNonChromeScheme) {
  auto entry = CreateEntry(GURL("https://example.com"));
  EXPECT_EQ(u"example.com", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL(base::StrCat({kBraveUIScheme, "://settings"})));
  EXPECT_EQ(UIUrl("settings"), entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("http://chrome.com"));
  EXPECT_EQ(u"chrome.com", entry->GetTitleForDisplay());

  entry = CreateEntry(GURL("http://example.com/?chrome://settings"));
  EXPECT_EQ(u"example.com/?chrome://settings", entry->GetTitleForDisplay());
}

}  // namespace content
