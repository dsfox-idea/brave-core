/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_MANAGER_FACTORY_H_
#define BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_MANAGER_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace growser {

class WarmNewTabPageManager;

class WarmNewTabPageManagerFactory : public ProfileKeyedServiceFactory {
 public:
  static WarmNewTabPageManagerFactory* GetInstance();
  // Creates the manager for `profile` (and thus starts warming) if needed.
  static WarmNewTabPageManager* GetForProfile(Profile* profile);
  // Returns the existing manager for `profile`, or nullptr if none - never
  // creates one, so the tab-adoption path cannot warm in headless/automation.
  static WarmNewTabPageManager* GetForProfileIfExists(Profile* profile);

  WarmNewTabPageManagerFactory(const WarmNewTabPageManagerFactory&) = delete;
  WarmNewTabPageManagerFactory& operator=(const WarmNewTabPageManagerFactory&) =
      delete;

 private:
  friend class base::NoDestructor<WarmNewTabPageManagerFactory>;

  WarmNewTabPageManagerFactory();
  ~WarmNewTabPageManagerFactory() override;

  // ProfileKeyedServiceFactory:
  // Lazy on purpose (no ServiceIsCreatedWithBrowserContext override): warming
  // must be driven by real browser-window creation, not profile init. Creating
  // a warm WebContents for every profile - headless/automation included - both
  // wastes work and destabilizes a windowless browser (a stray ownerless
  // chrome://newtab tore down the headless browser the icon gate drives). The
  // window-tied trigger lands with the tab-adoption step.
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace growser

#endif  // BRAVE_BROWSER_NEW_TAB_WARM_NEW_TAB_PAGE_MANAGER_FACTORY_H_
