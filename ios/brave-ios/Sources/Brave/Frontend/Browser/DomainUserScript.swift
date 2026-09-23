// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShields
import Foundation
import Shared
import WebKit

enum DomainUserScript: CaseIterable {
  case braveSearchHelper
  // Growser-278: no braveTalkHelper - Brave Talk is out of the product.
  // Growser-283: no braveSkus - SKUS is out of the product.

  /// Initialize this script with a URL
  init?(for url: URL, isPrivateBrowsing: Bool) {
    // First we look for an exact domain match
    if let host = url.host,
      let found = Self.allCases.first(where: { $0.associatedDomains.contains(host) }),
      found.isAllowedInPrivateMode || !isPrivateBrowsing
    {
      self = found
      return
    }

    // If no matches, we look for a baseDomain (eTLD+1) match.
    if let baseDomain = url.baseDomain,
      let found = Self.allCases.first(where: { $0.associatedDomains.contains(baseDomain) }),
      found.isAllowedInPrivateMode || !isPrivateBrowsing
    {
      self = found
      return
    }

    return nil
  }

  /// The domains associated with this script.
  var associatedDomains: Set<String> {
    switch self {
    case .braveSearchHelper:
      return Set([
        "search.brave.com", "search.brave.software",
        "search.bravesoftware.com", "safesearch.brave.com",
        "safesearch.brave.software", "safesearch.bravesoftware.com",
        "search-dev-local.brave.com",
      ])
    // Growser-278: no .braveTalkHelper and its talk.brave.com hosts.
    // Growser-283: no .braveSkus and its account.brave.com hosts.
    }
  }

  var isAllowedInPrivateMode: Bool {
    switch self {
    case .braveSearchHelper:  // Growser-278
      return true
    }
  }
}
