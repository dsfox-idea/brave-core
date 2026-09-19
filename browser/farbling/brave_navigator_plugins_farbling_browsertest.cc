/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/path_service.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/webcompat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_shields::ControlType;

namespace {

constexpr char kPluginsLengthScript[] = "navigator.plugins.length;";
constexpr char kNavigatorPdfViewerEnabledCrashTest[] =
    "navigator.pdfViewerEnabled == navigator.pdfViewerEnabled";
constexpr char kGetPluginsAsStringScript[] =
    "Array.from(navigator.plugins).map(p => p.name).join(',');";
// Growser-82: the five plugins every Chrome reports, in the order the HTML
// spec fixes. Not a farbling baseline - this is what a page must see at every
// fingerprinting level, measured in a running browser.
constexpr char kChromePluginNames[] =
    "PDF Viewer,Chrome PDF Viewer,Chromium PDF Viewer,Microsoft Edge PDF "
    "Viewer,WebKit built-in PDF";
constexpr int kChromePluginCount = 5;

}  // namespace

class BraveNavigatorPluginsFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveNavigatorPluginsFarblingBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        {
            brave_shields::features::kBraveShowStrictFingerprintingMode,
            webcompat::features::kBraveWebcompatExceptionsService,
        },
        {});
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    content::SetupCrossSiteRedirector(embedded_test_server());

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());

    top_level_page_url_ = embedded_test_server()->GetURL("a.com", "/");
    farbling_url_ = embedded_test_server()->GetURL("a.com", "/simple.html");
  }

  const GURL& farbling_url() { return farbling_url_; }

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(
        browser()->GetProfile());
  }

  void AllowFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW, top_level_page_url_);
  }

  void BlockFingerprinting() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK, top_level_page_url_);
  }

  void SetFingerprintingDefault() {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT, top_level_page_url_);
  }

  content::WebContents* contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

 private:
  GURL top_level_page_url_;
  GURL farbling_url_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Tests that access to navigator.pdfViewerEnabled attribute does not crash.
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       NavigatorPdfViewerEnabledNoCrash) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
  EXPECT_EQ(true, EvalJs(contents(), kNavigatorPdfViewerEnabledCrashTest));
}

// Growser-82: navigator.plugins is NOT farbled here, and this is the gate on
// that decision.
//
// Brave renames the built-in PDF plugins per origin, appends two invented ones
// and shuffles the order; we removed all of it - the reasoning lives beside the
// override, in
// chromium_src/third_party/blink/renderer/modules/plugins/dom_plugin_array.cc.
// In short: every Chrome on every machine reports the same five plugins with
// the same names, so noise there hides nobody. It marks them. A site that sees
// seven plugins with names no Chrome has ever produced learns more in one
// property read than the real list would ever have told it.
//
// The three tests this replaces asserted the farbled lists (brave-browser
// #9435, #10597 and #11278) and can only pass on a browser that farbles.
IN_PROC_BROWSER_TEST_F(BraveNavigatorPluginsFarblingBrowserTest,
                       NavigatorPluginsAreNeverFarbled) {
  auto expect_chrome_plugins = [&](const char* level) {
    SCOPED_TRACE(level);
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), farbling_url()));
    EXPECT_EQ(content::EvalJs(contents(), kPluginsLengthScript),
              kChromePluginCount);
    EXPECT_EQ(content::EvalJs(contents(), kGetPluginsAsStringScript),
              kChromePluginNames);
  };

  AllowFingerprinting();
  expect_chrome_plugins("fingerprinting allowed");

  SetFingerprintingDefault();
  expect_chrome_plugins("fingerprinting balanced (the default)");

  BlockFingerprinting();
  expect_chrome_plugins("fingerprinting blocked (strict)");

  // And with the webcompat exception a site can be granted: there is nothing
  // left for it to turn off, so the list must not move for that either.
  SetFingerprintingDefault();
  brave_shields::SetWebcompatEnabled(
      content_settings(), ContentSettingsType::BRAVE_WEBCOMPAT_PLUGINS, true,
      farbling_url(), nullptr);
  expect_chrome_plugins("webcompat exception enabled");
}
