// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Growser-280: no BraveVPN or GuardianConnect.
import Combine

/// The current status of the Brave VPN connection
enum VPNStatus: Equatable {
  /// VPN is not connected
  case disconnected
  /// VPN is connected to a given region
  case connected(activeRegion: VPNRegion, isSmartProxyRoutingEnabled: Bool)
}

/// The VPN region details to show
struct VPNRegion: Equatable {
  /// The unicode/emoji flag for the given country code provided
  var flag: String
  /// The display name for the connected server
  var displayName: String
  /// A boolean value indicates if this region supports smart proxy
  var smartProxySupported: Bool

  init(
    countryCode: String,
    displayName: String,
    smartProxySupported: Bool
  ) {
    self.flag = Self.flagEmojiForCountryCode(code: countryCode)
    self.displayName = displayName
    self.smartProxySupported = smartProxySupported
  }

  private static func flagEmojiForCountryCode(code: String) -> String {
    // Regional indicator symbol root Unicode flags index
    let rootIndex: UInt32 = 127397
    var unicodeScalarView = ""

    for scalar in code.unicodeScalars {
      // Shift the letter index to the flags index
      if let appendedScalar = UnicodeScalar(rootIndex + scalar.value) {
        // Append symbol to the Unicode string
        unicodeScalarView.unicodeScalars.append(appendedScalar)
      }
    }
    return unicodeScalarView
  }
}

// Growser-280: no live values - the VPN is out of the product, so the menu's
// status is always .disconnected (BrowserMenuController).
