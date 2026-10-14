/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/core/controller_client.h"

#include <memory>
#include <string>

#include "build/build_config.h"
#include "components/security_interstitials/core/metrics_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace security_interstitials {
namespace {

// Only the base URL is under test; nothing here is ever called.
class StubControllerClient : public ControllerClient {
 public:
  StubControllerClient() : ControllerClient(nullptr) {}

  bool CanLaunchDateAndTimeSettings() override { return false; }
  void LaunchDateAndTimeSettings() override {}
  void GoBack() override {}
  bool CanGoBack() override { return false; }
  bool CanGoBackBeforeNavigation() override { return false; }
  void GoBackAfterNavigationCommitted() override {}
  void Proceed() override {}
  void Reload() override {}
  void ShowCertificateViewer() override {}
  void OpenUrlInCurrentTab(const GURL& url) override {}
  void OpenUrlInNewForegroundTab(const GURL& url) override {}
  void OpenEnhancedProtectionSettings() override {}
#if BUILDFLAG(IS_ANDROID)
  void OpenAdvancedProtectionSettings() override {}
#endif
  PrefService* GetPrefService() override { return nullptr; }
  const std::string& GetApplicationLocale() const override { return locale_; }

 protected:
  const std::string GetExtendedReportingPrefName() const override {
    return std::string();
  }

 private:
  std::string locale_;
};

}  // namespace

// growser#344: "Learn more" on a warning page leads to growser.org, not to
// Google's Chrome help (#81).
TEST(GrowserControllerClientTest, HelpCenterIsOurs) {
  StubControllerClient client;
  EXPECT_EQ(client.GetBaseHelpCenterUrl(),
            GURL("https://growser.org/features.html#security-warnings"));
}

}  // namespace security_interstitials
