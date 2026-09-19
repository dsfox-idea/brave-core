// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_test_utils.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

namespace {

// Growser-82: these tests are about the farbling TOKEN - that it is stable,
// that it survives a restart or not, that clearing site data replaces it. The
// token has to be observed through some farbled surface, and Brave's choice
// was navigator.plugins, which we no longer farble at all (the reasoning is
// beside the override, chromium_src/.../plugins/dom_plugin_array.cc). Canvas
// readback is the surface that carries the token here: PerturbPixels keys the
// flipped bits on HMAC(farbling_token, pixels), so the same token gives the
// same data URL and a different token gives a different one. Measured in a
// running browser before it was written down - identical across two reads of
// one origin, different across two origins.
//
// No text is drawn on purpose: PRE_FarblingTokenBehaviourAfterRestart compares
// across a browser restart, and font rasterization is one more thing that could
// differ there. Flat rectangles leave the farbling as the only variable.
inline constexpr char kFarblingCanvasScript[] =
    "(() => {"
    "  const canvas = document.createElement('canvas');"
    "  canvas.width = 64; canvas.height = 16;"
    "  const ctx = canvas.getContext('2d');"
    "  ctx.fillStyle = '#f60'; ctx.fillRect(0, 0, 64, 16);"
    "  ctx.fillStyle = '#069'; ctx.fillRect(8, 4, 48, 8);"
    "  return canvas.toDataURL();"
    "})();";
inline constexpr char kFarblingProbeFilename[] = "farbling_canvas.txt";

}  // namespace

class BraveFarblingBrowserTest : public InProcessBrowserTest,
                                 public testing::WithParamInterface<bool> {
 public:
  BraveFarblingBrowserTest() {
    if (GetParam()) {
      scoped_feature_list_.InitWithFeatures(
          {webcompat::features::kBraveWebcompatExceptionsService,
           brave_shields::features::kBraveFarblingTokenReset},
          {});
    } else {
      scoped_feature_list_.InitWithFeatures(
          {webcompat::features::kBraveWebcompatExceptionsService},
          {brave_shields::features::kBraveFarblingTokenReset});
    }
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  const GURL& farbling_url() { return farbling_url_; }

  // A convinient function to return the underlying feature flag value to avoid
  // writing GetParam from within the test body which adds an extra layer of
  // parsing for readers
  bool IsFarblingTokenResetEnabled() const { return GetParam(); }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        browser()->GetProfile());
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  // By default farbling tokens are stable in tests. Passing 0 makes farbling
  // tokens be random even in tests.
  brave_shields::ScopedStableFarblingTokensForTesting
      scoped_random_farbling_tokens_{0, base::Token()};
  base::test::ScopedFeatureList scoped_feature_list_;
  GURL top_level_page_url_;
  GURL farbling_url_;
};

INSTANTIATE_TEST_SUITE_P(
    /* no prefix */,
    BraveFarblingBrowserTest,
    testing::Bool(),
    [](const testing::TestParamInfo<bool>& info) {
      return info.param ? "BraveFarblingBrowserTest_FarblingTokenResetEnabled"
                        : "BraveFarblingBrowserTest_FarblingTokenResetDisabled";
    });

IN_PROC_BROWSER_TEST_P(BraveFarblingBrowserTest,
                       PRE_FarblingTokenBehaviourAfterRestart) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  auto canvas_str = content::EvalJs(contents(), kFarblingCanvasScript);
  EXPECT_NE(canvas_str, "");
  // Write the current canvas readback to a file in the profile directory.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath temp_dir = browser()->GetProfile()->GetPath();
  base::FilePath output_file = temp_dir.AppendASCII(kFarblingProbeFilename);
  std::string result = canvas_str.ExtractString();
  base::WriteFile(output_file, result);
}

IN_PROC_BROWSER_TEST_P(BraveFarblingBrowserTest,
                       FarblingTokenBehaviourAfterRestart) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  auto canvas_str = content::EvalJs(contents(), kFarblingCanvasScript);
  EXPECT_NE(canvas_str, "");
  // Read the canvas readback from a file in the profile directory.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath temp_dir = browser()->GetProfile()->GetPath();
  base::FilePath input_file = temp_dir.AppendASCII(kFarblingProbeFilename);
  std::string previous_value;
  EXPECT_TRUE(base::ReadFileToString(input_file, &previous_value));
  // Compare against the readback from the previous launch.
  if (IsFarblingTokenResetEnabled()) {
    EXPECT_NE(canvas_str, previous_value);
  } else {
    EXPECT_EQ(canvas_str, previous_value);
  }
}

IN_PROC_BROWSER_TEST_P(BraveFarblingBrowserTest,
                       FarblingTokenIsClearedAfterWebsiteClear) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  const std::string canvas_before_cleanup =
      content::EvalJs(contents(), kFarblingCanvasScript).ExtractString();

  // Ensure that the farbling token is stable while the website data is not
  // cleared.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_EQ(content::EvalJs(contents(), kFarblingCanvasScript),
            canvas_before_cleanup);

  // Clear the website data.
  content_settings()->ClearSettingsForOneType(
      ContentSettingsType::BRAVE_SHIELDS_METADATA);

  // A new token should be generated.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_NE(content::EvalJs(contents(), kFarblingCanvasScript),
            canvas_before_cleanup);
}

IN_PROC_BROWSER_TEST_P(BraveFarblingBrowserTest,
                       CheckBetweenNormalAndIncognitoProfile) {
  auto* profile1 = browser()->GetProfile();
  auto* incognito_profile = CreateIncognitoBrowser(profile1)->GetProfile();

  auto* shields_settings_service =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile1);
  ASSERT_TRUE(shields_settings_service);

  auto* shields_settings_service_incognito =
      BraveShieldsSettingsServiceFactory::GetForProfile(incognito_profile);
  ASSERT_TRUE(shields_settings_service_incognito);

  // Compare the state of the PRNGs.
  brave_shields::FarblingPRNG prng;
  brave_shields::FarblingPRNG prng_incognito;
  EXPECT_TRUE(shields_settings_service->MakePseudoRandomGeneratorForURL(
      farbling_url(), {}, &prng));
  EXPECT_TRUE(
      shields_settings_service_incognito->MakePseudoRandomGeneratorForURL(
          farbling_url(), {}, &prng_incognito));
  EXPECT_NE(prng, prng_incognito);

  // Compare the farbling tokens.
  const auto farbling_token =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile1)
          ->GetFarblingToken(farbling_url(), {});
  const auto farbling_token_incognito =
      BraveShieldsSettingsServiceFactory::GetForProfile(incognito_profile)
          ->GetFarblingToken(farbling_url(), {});
  EXPECT_FALSE(farbling_token.is_zero());
  EXPECT_FALSE(farbling_token_incognito.is_zero());
  EXPECT_NE(farbling_token, farbling_token_incognito);
}

IN_PROC_BROWSER_TEST_P(BraveFarblingBrowserTest, CheckBetweenTwoProfiles) {
  auto* profile_1 = browser()->GetProfile();
  ASSERT_TRUE(profile_1);

  // Create another profile.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  base::FilePath dest_path = profile_manager->user_data_dir();
  dest_path = dest_path.Append(FILE_PATH_LITERAL("Profile2"));
  Profile* profile_2 = nullptr;
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    profile_2 = profile_manager->GetProfile(dest_path);
  }
  ASSERT_TRUE(profile_2);
  auto* browser_2 = CreateBrowser(profile_2);
  ASSERT_TRUE(browser_2);

  auto* shields_settings_service_profile_1 =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile_1);
  ASSERT_TRUE(shields_settings_service_profile_1);

  auto* shields_settings_service_profile_2 =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile_2);
  ASSERT_TRUE(shields_settings_service_profile_2);

  // Compare the state of the PRNGs.
  brave_shields::FarblingPRNG prng_1;
  brave_shields::FarblingPRNG prng_2;
  EXPECT_TRUE(
      shields_settings_service_profile_1->MakePseudoRandomGeneratorForURL(
          farbling_url(), {}, &prng_1));
  EXPECT_TRUE(
      shields_settings_service_profile_2->MakePseudoRandomGeneratorForURL(
          farbling_url(), {}, &prng_2));
  EXPECT_NE(prng_1, prng_2);

  // Compare the farbling tokens.
  const auto farbling_token_1 =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile_1)
          ->GetFarblingToken(farbling_url(), {});
  const auto farbling_token_2 =
      BraveShieldsSettingsServiceFactory::GetForProfile(profile_2)
          ->GetFarblingToken(farbling_url(), {});
  EXPECT_FALSE(farbling_token_1.is_zero());
  EXPECT_FALSE(farbling_token_2.is_zero());
  EXPECT_NE(farbling_token_1, farbling_token_2);
}
