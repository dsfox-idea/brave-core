/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/aistream/aistream_service_factory.h"

#include <optional>
#include <utility>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/no_destructor.h"
#include "brave/browser/aistream/aistream_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"

namespace aistream {

// static
AistreamServiceFactory* AistreamServiceFactory::GetInstance() {
  static base::NoDestructor<AistreamServiceFactory> instance;
  return instance.get();
}

// static
AistreamService* AistreamServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<AistreamService*>(
      GetInstance()->GetServiceForBrowserContext(profile, /*create=*/true));
}

AistreamServiceFactory::AistreamServiceFactory()
    : ProfileKeyedServiceFactory("AistreamService",
                                 ProfileSelections::BuildForRegularProfile()) {}

AistreamServiceFactory::~AistreamServiceFactory() = default;

std::unique_ptr<KeyedService>
AistreamServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  std::optional<AistreamService::Config> config = AistreamService::ReadConfig(
      *base::CommandLine::ForCurrentProcess(), *base::Environment::Create());
  if (!config) {
    return nullptr;
  }
  return std::make_unique<AistreamService>(
      std::move(*config), context->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess());
}

}  // namespace aistream
