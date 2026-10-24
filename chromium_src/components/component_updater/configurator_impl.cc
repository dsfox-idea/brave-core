/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/configurator_impl.h"

#include "brave/components/update_client/privacy_preserving_protocol_handler.h"

#define EnabledBackgroundDownloader EnabledBackgroundDownloader_Unused
#define EnabledCupSigning EnabledCupSigning_Unused
#define ProtocolHandlerFactoryJSON PrivacyPreservingProtocolHandlerFactory
#define UpdateUrl UpdateUrl_ChromiumImpl
#include <components/component_updater/configurator_impl.cc>
#undef UpdateUrl
#undef ProtocolHandlerFactoryJSON
#undef EnabledCupSigning
#undef EnabledBackgroundDownloader

namespace component_updater {

bool ConfiguratorImpl::EnabledBackgroundDownloader() const {
  return false;
}

bool ConfiguratorImpl::EnabledCupSigning() const {
  return false;
}

// Growser-347: when the update URL is our backend, its auxiliary server on
// the same domain follows it in the list. update_client::RequestSender moves
// to the next URL on a network error, so a network that cannot reach the
// backend still updates components - this is what iOS relies on, where the
// browser's request hook does not run. Elsewhere the hook already moves the
// first URL to the mirror while the backend is unreachable.
std::vector<GURL> ConfiguratorImpl::UpdateUrl() const {
  std::vector<GURL> urls = UpdateUrl_ChromiumImpl();
  if (urls.size() == 1 && urls[0].host() == "backend.growser.org") {
    GURL::Replacements replacements;
    replacements.SetHostStr("ru.growser.org");
    urls.push_back(urls[0].ReplaceComponents(replacements));
  }
  return urls;
}

}  // namespace component_updater
