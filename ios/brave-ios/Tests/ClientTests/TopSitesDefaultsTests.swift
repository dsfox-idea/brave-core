// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import XCTest

@testable import Brave

/// growser#310: a fresh install's new tab page shows the most visited sites.
class TopSitesDefaultsTests: XCTestCase {
  func testMostVisitedIsTheDefaultMode() {
    XCTAssertEqual(Preferences.NewTabPage.topSitesMode.defaultValue, .mostVisited)
  }

  func testMostVisitedIsNotTurnedBackIntoFavourites() {
    // TopSitesTileSource shows favourites in place of most visited while
    // this feature is off.
    XCTAssertTrue(FeatureList.kTopsitesEnabled.enabled)
  }
}
