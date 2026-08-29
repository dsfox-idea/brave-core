/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/warm_new_tab_page_activator.h"

#include "base/check.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/function_ref.h"
#include "base/logging.h"
#include "brave/browser/new_tab/warm_new_tab_page_manager.h"
#include "brave/browser/new_tab/warm_new_tab_page_manager_factory.h"
#include "chrome/browser/headless/headless_mode_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_collection.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"
#include "content/public/common/content_switches.h"

namespace growser {

namespace {
WarmNewTabPageActivator* g_instance = nullptr;
}  // namespace

WarmNewTabPageActivator::WarmNewTabPageActivator() {
  GlobalBrowserCollection* collection = GlobalBrowserCollection::GetInstance();
  browser_collection_observation_.Observe(collection);
  // The startup window is usually already open by the time this runs, so
  // OnBrowserCreated never fires for it - warm any existing browser now.
  collection->ForEach([this](BrowserWindowInterface* browser) {
    WarmForBrowser(browser);
    return true;
  });
}

WarmNewTabPageActivator::~WarmNewTabPageActivator() = default;

// static
void WarmNewTabPageActivator::CreateInstance() {
  g_instance = new WarmNewTabPageActivator();
}

// static
bool WarmNewTabPageActivator::HasInstance() {
  return g_instance != nullptr;
}

// static
void WarmNewTabPageActivator::ClearInstance() {
  CHECK(g_instance);
  delete g_instance;
  g_instance = nullptr;
}

void WarmNewTabPageActivator::OnBrowserCreated(
    BrowserWindowInterface* browser) {
  WarmForBrowser(browser);
}

void WarmNewTabPageActivator::WarmForBrowser(BrowserWindowInterface* browser) {
  if (!base::FeatureList::IsEnabled(kWarmNewTabPage)) {
    return;
  }
  // Never warm a windowless browser (headless/automation): an ownerless warm
  // chrome://newtab tore down the headless browser the icon gate drives.
  if (headless::IsHeadlessMode() ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kEnableAutomation)) {
    return;
  }
  Browser* chrome_browser = browser->GetBrowserForMigrationOnly();
  if (!chrome_browser || !chrome_browser->SupportsWindowFeature(
                             Browser::WindowFeature::kFeatureTabStrip)) {
    return;
  }
  Profile* profile = browser->GetProfile();
  if (!profile) {
    return;
  }
  // Creating the service warms the page. The factory restricts to regular
  // profiles; one warm page per profile (a later window is a no-op).
  VLOG(1) << "WarmNTP: warming profile";
  WarmNewTabPageManagerFactory::GetForProfile(profile);
}

}  // namespace growser
