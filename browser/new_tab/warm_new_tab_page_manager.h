/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_MANAGER_H_
#define BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_MANAGER_H_

#include <memory>

#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

class Profile;

namespace growser {

// Gates the whole warm New Tab Page mechanism. Enabled by default; a kill
// switch while it is stabilized.
BASE_DECLARE_FEATURE(kWarmNewTabPage);

// Keeps one hidden, fully-rendered New Tab Page WebContents warm per profile so
// that opening a new tab shows an already-painted page instead of building it
// from scratch. The NTP is already process-per-site, so the win here is a
// pre-rendered, pre-bound WebContents (DOM + mojo + first paint), not a warm
// process.
//
// Lifecycle:
//  - One warm contents at a time, created hidden but not initially_hidden (so
//    it loads and paints in the background, the way WebUIContentsPreloadManager
//    warms top-chrome WebUIs).
//  - TakeWarmContents() hands the ready contents to the tab strip and schedules
//    a fresh one.
//  - The warm contents is rebuilt only when the top-sites composition changes
//    (wired in a later step) and dropped under memory pressure. Clock,
//    background and widgets keep their live mojo bindings while hidden, so they
//    update in place and do not force a rebuild.
class WarmNewTabPageManager : public KeyedService {
 public:
  explicit WarmNewTabPageManager(Profile* profile);
  WarmNewTabPageManager(const WarmNewTabPageManager&) = delete;
  WarmNewTabPageManager& operator=(const WarmNewTabPageManager&) = delete;
  ~WarmNewTabPageManager() override;

  // Returns the warm New Tab Page WebContents if one is ready to be shown, and
  // schedules a replacement. Returns nullptr if none is ready yet (the caller
  // then builds the page the ordinary way).
  std::unique_ptr<content::WebContents> TakeWarmContents();

  // Discards any warm contents and builds a fresh one. Called when the warm
  // page's data would be stale (top-sites composition change).
  void Rebuild();

  content::WebContents* warm_contents_for_testing() {
    return warm_contents_.get();
  }
  bool is_ready_for_testing() const { return is_ready_; }

 private:
  // Watches the warm contents for its first visually non-empty paint.
  class PaintWatcher;

  // KeyedService:
  void Shutdown() override;

  void BuildWarmContents();
  void OnWarmContentsReady();

  const raw_ptr<Profile> profile_;

  std::unique_ptr<content::WebContents> warm_contents_;
  std::unique_ptr<PaintWatcher> paint_watcher_;
  bool is_ready_ = false;

  base::WeakPtrFactory<WarmNewTabPageManager> weak_factory_{this};
};

}  // namespace growser

#endif  // BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_MANAGER_H_
