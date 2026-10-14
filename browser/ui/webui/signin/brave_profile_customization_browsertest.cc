// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/webui/custom_profile_image/features.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/web_ui_mocha_browser_test.h"
#include "content/public/test/browser_test.h"
#include "ui/base/ui_base_switches.h"

namespace {

class BraveProfileCustomizationWebUITest : public WebUIMochaBrowserTest {
 protected:
  explicit BraveProfileCustomizationWebUITest(bool feature_enabled) {
    scoped_feature_list_.InitWithFeatureState(
        custom_profile_image::features::kBraveCustomProfileImage,
        feature_enabled);
    set_test_loader_host(chrome::kChromeUIProfileCustomizationHost);
  }

  // Growser-352: the mocha suites compare against English strings, and our
  // browser starts in the machine's language - Russian here.
  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebUIMochaBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kLang, "en-US");
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

class BraveProfileCustomizationFeatureDisabledWebUITest
    : public BraveProfileCustomizationWebUITest {
 public:
  BraveProfileCustomizationFeatureDisabledWebUITest()
      : BraveProfileCustomizationWebUITest(false) {}
};

class BraveProfileCustomizationFeatureEnabledWebUITest
    : public BraveProfileCustomizationWebUITest {
 public:
  BraveProfileCustomizationFeatureEnabledWebUITest()
      : BraveProfileCustomizationWebUITest(true) {}
};

IN_PROC_BROWSER_TEST_F(BraveProfileCustomizationFeatureDisabledWebUITest,
                       DoesNotRenderCustomProfileImageRow) {
  RunTest("signin/profile_customization_test.js",
          "runMochaSuite('BraveProfileCustomizationFeatureDisabledTests')");
}

IN_PROC_BROWSER_TEST_F(BraveProfileCustomizationFeatureEnabledWebUITest,
                       RendersAndUsesCustomProfileImageRow) {
  RunTest("signin/profile_customization_test.js",
          "runMochaSuite('BraveProfileCustomizationFeatureEnabledTests')");
}

IN_PROC_BROWSER_TEST_F(BraveProfileCustomizationFeatureEnabledWebUITest,
                       PreservesLocalProfileCreation) {
  RunTest("signin/profile_customization_test.js",
          "runMochaSuite('LocalProfileCreationTest')");
}

}  // namespace
