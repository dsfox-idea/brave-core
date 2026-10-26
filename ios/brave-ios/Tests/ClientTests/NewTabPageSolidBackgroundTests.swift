// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import UIKit
import XCTest

@testable import Brave

/// growser#392: with background images off the new tab page is a flat theme
/// colour, the desktop's, and the favourites read on it in the light theme.
class NewTabPageSolidBackgroundTests: XCTestCase {
  private let light = UITraitCollection(userInterfaceStyle: .light)
  private let dark = UITraitCollection(userInterfaceStyle: .dark)

  private func hex(_ color: UIColor) -> Int {
    var r: CGFloat = 0
    var g: CGFloat = 0
    var b: CGFloat = 0
    var a: CGFloat = 0
    XCTAssertTrue(color.getRed(&r, green: &g, blue: &b, alpha: &a))
    return Int((r * 255).rounded()) << 16 | Int((g * 255).rounded()) << 8
      | Int((b * 255).rounded())
  }

  func testFlatBackgroundIsTheDesktopColour() {
    let color = NewTabPageBackgroundView().backgroundColor!
    XCTAssertEqual(hex(color.resolvedColor(with: light)), 0xf4f4f5)
    XCTAssertEqual(hex(color.resolvedColor(with: dark)), 0x202124)
  }

  func testFavouriteLabelIsDarkInTheLightTheme() {
    let cell = TopSitesCell(frame: .zero)
    let label = cell.stackView.arrangedSubviews.compactMap { $0 as? UILabel }.first!
    // The cell no longer forces the dark theme on itself.
    XCTAssertEqual(cell.overrideUserInterfaceStyle, .unspecified)
    XCTAssertEqual(
      hex(label.textColor.resolvedColor(with: light)),
      hex(UIColor.label.resolvedColor(with: light))
    )
    XCTAssertEqual(hex(label.textColor.resolvedColor(with: dark)), 0xffffff)
  }
}
