// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveUI
import UIKit
import XCTest

/// growser#286: the browser's own mark is `growser.logo`; `brave.logo` stays
/// only as the Brave Shields button (#55). A name that does not resolve draws
/// nothing at all - SwiftUI's Image shows an empty frame, and the NTP's
/// favicon falls back - so the names are pinned here.
class GrowserImagesTests: XCTestCase {
  func testTheBrowserMarkResolves() throws {
    let logo = try XCTUnwrap(UIImage(sharedNamed: "growser.logo"))
    XCTAssertEqual(logo.size, CGSize(width: 24, height: 24))
  }

  func testTheWordmarkIsOursUnderItsOldName() throws {
    // OnboardingStepView still asks for "brave.wordmark"; the picture is
    // "Growser", which is far wider than it is tall.
    let wordmark = try XCTUnwrap(UIImage(sharedNamed: "brave.wordmark"))
    XCTAssertGreaterThan(wordmark.size.width / wordmark.size.height, 4)
  }

  func testTheShieldsButtonKeepsBravesLion() {
    XCTAssertNotNil(UIImage(sharedNamed: "brave.logo"))
    XCTAssertNotNil(UIImage(sharedNamed: "brave.logo.greyscale"))
  }
}
