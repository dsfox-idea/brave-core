// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveWidgetsModels
import DesignSystem
import Foundation
import OrderedCollections
import Strings
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
    if !isWalletAvailable {
      options.remove(.wallet)
    }
    // Growser-279: Leo is out of the product, so its shortcuts always go.
    options.remove(.braveLeo)
    options.remove(.braveLeoVoiceInput)
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
    if !isWalletAvailable {
      disabled.insert(.wallet)
    }
    // Growser-279: Leo is out of the product, so its shortcuts are always off.
    disabled.insert(.braveLeo)
    disabled.insert(.braveLeoVoiceInput)
    return disabled
  }

  var displayString: String {
    switch self {
    case .unknown:
      return ""
    case .bookmarks:
      return Strings.bookmarksMenuItem
    case .history:
      return Strings.historyMenuItem
    case .downloads:
      return Strings.downloadsMenuItem
    case .playlist:
      return Strings.bravePlaylistItemTitle
    case .wallet:
      return Strings.Wallet.wallet
    case .braveNews:
      return Strings.braveNewsItemTitle
    case .braveLeo:
      return Strings.leoMenuItem
    case .askBrave:
      return Strings.askBraveMenuItem
    case .braveLeoVoiceInput:
      return Strings.leoVoiceInputMenuItem
    default:
      return ""
    }
  }

  var braveSystemImageName: String? {
    switch self {
    case .unknown:
      return nil
    case .newTab:
      return "leo.browser.mobile-tab-new"
    case .newPrivateTab:
      return "leo.product.private-window"
    case .bookmarks:
      return "leo.product.bookmarks"
    case .history:
      return "leo.history"
    case .downloads:
      return "leo.download"
    case .playlist:
      return "leo.product.playlist"
    case .search:
      return "leo.search"
    case .wallet:
      return "leo.product.brave-wallet"
    case .scanQRCode:
      return "leo.qr.code"
    case .braveNews:
      return "leo.product.brave-news"
    case .braveLeo, .askBrave:
      return "leo.product.brave-leo"
    case .braveLeoVoiceInput:
      return "leo.microphone"
    @unknown default:
      return nil
    }
  }

  var image: UIImage? {
    return braveSystemImageName.flatMap { UIImage(braveSystemNamed: $0) }
  }
}
