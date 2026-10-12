// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveWidgetsModels
import DesignSystem
import Foundation
import OrderedCollections
import UIKit

extension WidgetShortcut {
  static func eligibleButtonShortcuts(
    prefs: any PrefService,
    isWalletAvailable: Bool
  ) -> OrderedSet<WidgetShortcut> {
    var options = OrderedSet<WidgetShortcut>([
      .bookmarks,
      .history,
      .downloads,
      .playlist,
      .wallet,
      .braveNews,
      .braveLeo,
      .askBrave,
      .braveLeoVoiceInput,
    ])
    options.remove(.playlist)  // Growser-282: Playlist is out of the product.
    options.remove(.braveNews)  // Growser-281: Brave News is out of the product.
    options.remove(.wallet)  // Growser-287: the wallet is out of the product.
    // Growser-279: Leo is out of the product, so its shortcuts always go.
    options.remove(.braveLeo)
    options.remove(.braveLeoVoiceInput)
    options.remove(.askBrave)  // Growser-292: it opened Brave Search.
    return options
  }

  /// The set of shortcuts that are currently unavailable and should be hidden from the Shortcuts
  /// widgets (for example, disabled by a Brave Origin or enterprise policy).
  static func disabledWidgetShortcuts(
    prefs: any PrefService,
    isWalletAvailable: Bool
  ) -> Set<WidgetShortcut> {
    var disabled: Set<WidgetShortcut> = []
    disabled.insert(.playlist)  // Growser-282: Playlist is out of the product.
    disabled.insert(.braveNews)  // Growser-281: Brave News is out of the product.
    disabled.insert(.wallet)  // Growser-287: the wallet is out of the product.
    // Growser-279: Leo is out of the product, so its shortcuts are always off.
    disabled.insert(.braveLeo)
    disabled.insert(.braveLeoVoiceInput)
    disabled.insert(.askBrave)  // Growser-292: it opened Brave Search.
    return disabled
  }

  var image: UIImage? {
    return braveSystemImageName.flatMap { UIImage(braveSystemNamed: $0) }
  }
}
