/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/common/extension_urls.h"

#include "base/command_line.h"
#include "brave/components/update_client/buildflags.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/component_updater/component_updater_switches.h"
#include "content/public/test/browser_test.h"
// Growser-366
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/test/url_loader_interceptor.h"
#include "extensions/browser/install_approval.h"
#include "extensions/browser/webstore_installer.h"

using ExtensionUrlsBrowserTest = PlatformBrowserTest;

// Growser-366: a Web Store install downloads from the Web Store's own update
// URL; update checks stay on the url-source.
IN_PROC_BROWSER_TEST_F(ExtensionUrlsBrowserTest, InstallsDownloadFromWebStore) {
  constexpr char kId[] = "ddkjiahejlhfcafbddmgiahcphecmpfh";
  GURL requested;
  content::URLLoaderInterceptor interceptor(base::BindLambdaForTesting(
      [&](content::URLLoaderInterceptor::RequestParams* params) {
        if (params->url_request.url.query().find(kId) == std::string::npos) {
          return false;
        }
        requested = params->url_request.url;
        content::URLLoaderInterceptor::WriteResponse(
            "HTTP/1.1 404 Not Found\n\n", "", params->client.get());
        return true;
      }));

  content::BrowserContext* context = chrome_test_utils::GetProfile(this);
  base::RunLoop run_loop;
  auto installer = base::MakeRefCounted<extensions::WebstoreInstaller>(
      context, base::BindOnce([](const std::string&) {}),
      base::BindLambdaForTesting(
          [&](const std::string&, const std::string&,
              extensions::WebstoreInstaller::FailureReason) {
            run_loop.Quit();
          }),
      chrome_test_utils::GetActiveWebContents(this), kId,
      extensions::InstallApproval::CreateWithNoInstallPrompt(
          context, kId,
          base::DictValue()
              .Set("name", "Growser-366")
              .Set("manifest_version", 3)
              .Set("version", "1.0"),
          false),
      extensions::WebstoreInstaller::INSTALL_SOURCE_OTHER);
  installer->Start();
  run_loop.Run();

  const GURL store(extension_urls::kChromeWebstoreUpdateURL);
  EXPECT_EQ(store.host(), requested.host());
  EXPECT_EQ(store.path(), requested.path());
  EXPECT_EQ(GURL(BUILDFLAG(UPDATER_PROD_ENDPOINT)),
            extension_urls::GetWebstoreUpdateUrl());
}

IN_PROC_BROWSER_TEST_F(ExtensionUrlsBrowserTest, IsWebstoreUpdateUrl) {
  GURL url = GURL(extension_urls::kChromeWebstoreUpdateURL);
  EXPECT_TRUE(extension_urls::IsWebstoreUpdateUrl(url));

  url = GURL(BUILDFLAG(UPDATER_PROD_ENDPOINT));
  EXPECT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kComponentUpdater));
  EXPECT_TRUE(extension_urls::IsWebstoreUpdateUrl(url));
}
