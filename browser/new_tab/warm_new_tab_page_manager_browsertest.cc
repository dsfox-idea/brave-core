/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/warm_new_tab_page_manager.h"

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/run_until.h"
#include "base/time/time.h"
#include "brave/browser/new_tab/warm_new_tab_page_manager_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/top_sites.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "url/gurl.h"

namespace growser {

// growser#354: the most-visited service notifies its observers on every
// query, whether the tiles changed or not. The warm page has to survive a
// repeat - each rebuild is a whole New Tab Page load - and still rebuild when
// the tiles really change.
class WarmNewTabPageManagerBrowserTest : public InProcessBrowserTest {
 protected:
  Profile* profile() { return browser()->GetProfile(); }
};

IN_PROC_BROWSER_TEST_F(WarmNewTabPageManagerBrowserTest,
                       RebuildsOnlyWhenTheTilesChange) {
  // Browser tests skip the activator, so ask the factory for the page.
  WarmNewTabPageManager* warm =
      WarmNewTabPageManagerFactory::GetForProfile(profile());
  ASSERT_TRUE(warm);
  ASSERT_TRUE(base::test::RunUntil([&] {
    return warm->tiles_updates_for_testing() >= 1 &&
           warm->is_ready_for_testing();
  }));
  ASSERT_TRUE(warm->warm_contents_for_testing());
  base::WeakPtr<content::WebContents> built =
      warm->warm_contents_for_testing()->GetWeakPtr();

  // Blocking a URL that is not a tile makes TopSites notify, and the service
  // hands the same tiles over again.
  scoped_refptr<history::TopSites> top_sites =
      TopSitesFactory::GetForProfile(profile());
  ASSERT_TRUE(top_sites);
  const int before_repeat = warm->tiles_updates_for_testing();
  top_sites->AddBlockedUrl(GURL("https://not-a-tile.example/"));
  ASSERT_TRUE(base::test::RunUntil([&] {
    return warm->tiles_updates_for_testing() > before_repeat;
  }));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(built) << "unchanged tiles rebuilt the warm page";

  // A visited page that TopSites picks up is a real change.
  HistoryServiceFactory::GetForProfile(profile(),
                                       ServiceAccessType::EXPLICIT_ACCESS)
      ->AddPage(GURL("https://tile.example/"), base::Time::Now(),
                history::SOURCE_BROWSED);
  top_sites->SyncWithHistory();
  EXPECT_TRUE(base::test::RunUntil([&] { return !built; }))
      << "changed tiles did not rebuild the warm page";
}

}  // namespace growser
