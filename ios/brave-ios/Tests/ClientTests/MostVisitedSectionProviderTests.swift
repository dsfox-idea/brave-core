// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import XCTest

@testable import Brave

/// growser#310: which of the most visited sites the new tab page shows.
class MostVisitedSectionProviderTests: XCTestCase {
  private func tile(_ url: String) -> NTPTile {
    NTPTile(url: URL(string: url)!, title: url)
  }

  private func urls(_ tiles: [NTPTile]) -> [String] {
    tiles.map(\.url.absoluteString)
  }

  func testAFavouriteIsNotShownTwice() {
    let tiles = [tile("https://a.example/"), tile("https://b.example/"), tile("https://c.example/")]
    let shown = MostVisitedSectionProvider.visible(
      tiles,
      favourites: ["https://b.example/"],
      capacity: 10
    )
    XCTAssertEqual(urls(shown), ["https://a.example/", "https://c.example/"])
  }

  func testTheRankingOrderIsKeptAndCutToWhatFits() {
    let tiles = (1...12).map { tile("https://\($0).example/") }
    let shown = MostVisitedSectionProvider.visible(tiles, favourites: [], capacity: 10)
    XCTAssertEqual(urls(shown), (1...10).map { "https://\($0).example/" })
  }

  func testNoRoomShowsNothing() {
    let shown = MostVisitedSectionProvider.visible([tile("https://a.example/")], favourites: [], capacity: 0)
    XCTAssertTrue(shown.isEmpty)
  }

  func testTwoRowsAtMost() {
    XCTAssertEqual(MostVisitedSectionProvider.rows, 2)
  }
}
