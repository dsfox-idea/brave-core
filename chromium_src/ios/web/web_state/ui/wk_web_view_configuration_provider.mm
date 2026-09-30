/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/web/web_state/ui/wk_web_view_configuration_provider.h"

#include "base/notreached.h"
#include "base/supports_user_data.h"
#include "ios/web/public/web_client.h"

// Replace the `WKWebViewConfigurationProvider` constructor call with the Brave
// subclass inside of `WKWebViewConfigurationProvider::FromBrowserState`
#define SetUserData(key, ...)                                                \
  SetUserData(key, base::WrapUnique(new BraveWKWebViewConfigurationProvider( \
                       browser_state)));
#include <ios/web/web_state/ui/wk_web_view_configuration_provider.mm>
#undef SetUserData

namespace web {

void BraveWKWebViewConfigurationProvider::ResetWithWebViewConfiguration(
    WKWebViewConfiguration* configuration) {
  if (configuration != nil) {
    // We need to ensure that each tab has isolated WKUserContentController &
    // WKPreferences, because as of now we specifically adjust these values per
    // web view created rather than when the configuration is created.
    //
    // This must happen prior to WKWebView's creation.
    configuration.userContentController =
        [[WKUserContentController alloc] init];
    configuration.preferences = [configuration.preferences copy];
  }

  WKWebViewConfigurationProvider::ResetWithWebViewConfiguration(configuration);

  // Adjusts the underlying WKWebViewConfiguration for settings we don't want
  // to inherit from Chromium

  // Restore WKWebView long press
  @try {
    [configuration_ setValue:@YES forKey:@"longPressActionsEnabled"];
  } @catch (NSException* exception) {
    NOTREACHED() << "Error setting value for longPressActionsEnabled";
  }

  // Growser-343: Chromium's Safe Browsing (SafeBrowsingTabHelper, local list
  // through our backend) replaces Apple's, so WebKit's warning stays off here,
  // before any web view copies this configuration.
  [[configuration_ preferences] setFraudulentWebsiteWarningEnabled:NO];

  // Reset fullscreen to default as it wasn't set in Brave
  [[configuration_ preferences] setElementFullscreenEnabled:NO];

  // Add Brave-specific adjustments to the WKWebViewConfiguration here

  configuration_.dataDetectorTypes = WKDataDetectorTypePhoneNumber;

  // Explicitly pass in the private configuration so that it can be mutated
  // correctly prior to being used in WebState. This is a temporary measure
  // and can be removed once:
  // - The `internal` scheme is removed and interstitials are converted to WebUI
  //   https://github.com/brave/brave-browser/issues/53028
  // - We replace WebKit's built in safe browsing implementation with Chromiums
  //   https://github.com/brave/brave-browser/issues/53029
  // - We replace our custom HTTPS-only upgrade and can remove the assignment of
  //   `WKWebViewConfiguration.upgradeKnownHostsToHTTPS`
  //   https://github.com/brave/brave-browser/issues/53030
  GetWebClient()->DidResetConfiguration(browser_state_, configuration_);
}

}  // namespace web
