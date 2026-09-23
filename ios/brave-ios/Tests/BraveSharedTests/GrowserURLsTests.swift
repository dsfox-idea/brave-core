// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import XCTest

/// growser#299: the links a person can reach from Settings, the new tab page
/// and the Shields panel lead to growser.org, never to Brave (#81).
class GrowserURLsTests: XCTestCase {
  func testReachableLinksAreOurs() {
    let reachable: [String: URL] = [
      "report a bug": URL.brave.community,
      "privacy policy": URL.brave.privacy,
      "privacy features": URL.brave.privacyFeatures,
      "Safe Browsing help": URL.brave.safeBrowsingHelp,
    ]
    for (name, url) in reachable {
      XCTAssertEqual(url.host, "growser.org", name)
      XCTAssertEqual(url.scheme, "https", name)
    }
  }

  func testThePrivacyPolicyIsThePageThatExists() {
    XCTAssertEqual(URL.brave.privacy.absoluteString, "https://growser.org/privacy.html")
  }
}
