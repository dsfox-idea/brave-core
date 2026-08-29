/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/warm_new_tab_page_manager_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "brave/browser/new_tab/warm_new_tab_page_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"

namespace growser {

// static
WarmNewTabPageManagerFactory* WarmNewTabPageManagerFactory::GetInstance() {
  static base::NoDestructor<WarmNewTabPageManagerFactory> instance;
  return instance.get();
}

// static
WarmNewTabPageManager* WarmNewTabPageManagerFactory::GetForProfile(
    Profile* profile) {
  return static_cast<WarmNewTabPageManager*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

WarmNewTabPageManagerFactory::WarmNewTabPageManagerFactory()
    : ProfileKeyedServiceFactory("WarmNewTabPageManager",
                                 ProfileSelections::BuildForRegularProfile()) {}

WarmNewTabPageManagerFactory::~WarmNewTabPageManagerFactory() = default;

std::unique_ptr<KeyedService>
WarmNewTabPageManagerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  if (!base::FeatureList::IsEnabled(kWarmNewTabPage)) {
    return nullptr;
  }
  Profile* profile = Profile::FromBrowserContext(context);
  return std::make_unique<WarmNewTabPageManager>(profile);
}

}  // namespace growser
