// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Preferences
import UIKit
import XCTest

@testable import Brave
@testable import Data

/// growser#394: the first tiles arrive while recent searches are shown.
/// `updateTiles()` sets `tiles` (so `availableSections` gains `.topSites`) and
/// then `updateUIWithSnapshot()` refetches recent searches before applying its
/// own snapshot. The fetched-results callback ran against the snapshot still
/// on screen, which has no `.topSites` section, and
/// `itemIdentifiers(inSection:)` threw - the crash of 26.1024 (0).
class TopSitesRecentSearchesTests: XCTestCase {
  override func setUp() {
    super.setUp()
    DataController.shared = InMemoryDataController()
    Preferences.Search.shouldShowRecentSearches.value = true
    Preferences.NewTabPage.topSitesMode.value = .mostVisited
  }

  override func tearDown() {
    Preferences.Search.shouldShowRecentSearches.reset()
    Preferences.NewTabPage.topSitesMode.reset()
    super.tearDown()
  }

  func testFirstTilesArriveWhileRecentSearchesAreShown() {
    let saved = expectation(forNotification: .NSManagedObjectContextDidSave, object: nil)
    RecentSearch.addItem(type: .text, text: "sozcu", websiteUrl: nil)
    wait(for: [saved], timeout: 5)
    XCTAssertEqual(RecentSearch.totalCount(), 1)

    // No most visited sites yet: the page opens with recent searches only.
    let tileSource = TopSitesTileSource(mostVisitedSites: nil, isPrivateBrowsing: false)
    let controller = TopSitesViewController(
      privateBrowsingManager: PrivateBrowsingManager(),
      defaultSearchEngine: nil,
      tileSource: tileSource,
      topSiteAction: { _ in },
      recentSearchAction: { _, _ in }
    )
    controller.loadViewIfNeeded()
    XCTAssertEqual(controller.availableSections, [.recentSearches])

    // The first most visited tiles arrive.
    tileSource.mostVisitedSitesDidUpdateTiles([
      NTPTile(url: URL(string: "https://www.sozcu.com.tr/")!, title: "Sözcü")
    ])
    let updated = expectation(description: "the controller took the new tiles")
    DispatchQueue.main.async { updated.fulfill() }
    wait(for: [updated], timeout: 5)

    XCTAssertEqual(controller.availableSections, [.topSites, .recentSearches])
    XCTAssertEqual(controller.tiles.count, 1)
  }
}
