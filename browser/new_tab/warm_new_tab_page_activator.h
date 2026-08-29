/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_ACTIVATOR_H_
#define BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_ACTIVATOR_H_

#include "base/scoped_observation.h"
#include "chrome/browser/ui/browser_window/public/browser_collection_observer.h"

class BrowserCollection;
class BrowserWindowInterface;

namespace growser {

// Warms the New Tab Page for a profile when a real, non-headless tabbed browser
// window is created. A leaking process-lifetime singleton created from
// BraveBrowserMainExtraParts (mirrors brave::BraveWindowTracker).
//
// Warming is tied to window creation, never to profile init: an ownerless warm
// chrome://newtab created for a windowless headless/automation browser tore
// that browser down (it is the browser the icon gate drives), so those are
// skipped here.
class WarmNewTabPageActivator : public BrowserCollectionObserver {
 public:
  WarmNewTabPageActivator();
  WarmNewTabPageActivator(const WarmNewTabPageActivator&) = delete;
  WarmNewTabPageActivator& operator=(const WarmNewTabPageActivator&) = delete;
  ~WarmNewTabPageActivator() override;

  static void CreateInstance();
  static bool HasInstance();
  static void ClearInstance();

 private:
  // BrowserCollectionObserver:
  void OnBrowserCreated(BrowserWindowInterface* browser) override;

  // Warms the New Tab Page for `browser`'s profile if it is a real, non-headless
  // tabbed window. Shared by OnBrowserCreated and the startup sweep.
  void WarmForBrowser(BrowserWindowInterface* browser);

  base::ScopedObservation<BrowserCollection, BrowserCollectionObserver>
      browser_collection_observation_{this};
};

}  // namespace growser

#endif  // BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_ACTIVATOR_H_
