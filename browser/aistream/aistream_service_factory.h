/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AISTREAM_AISTREAM_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_AISTREAM_AISTREAM_SERVICE_FACTORY_H_

#include <memory>

#include "chrome/browser/profiles/profile_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
class NoDestructor;
}  // namespace base

namespace aistream {

class AistreamService;

class AistreamServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static AistreamServiceFactory* GetInstance();
  // Null unless the browser was started with a valid --aistream-bridge.
  static AistreamService* GetForProfile(Profile* profile);

 private:
  friend base::NoDestructor<AistreamServiceFactory>;

  AistreamServiceFactory();
  ~AistreamServiceFactory() override;

  // ProfileKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override;
};

}  // namespace aistream

#endif  // BRAVE_BROWSER_AISTREAM_AISTREAM_SERVICE_FACTORY_H_
