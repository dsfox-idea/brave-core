// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import XCTest

@testable import Brave

private typealias SE = InitialSearchEngines

// Growser-292: these used to assert Brave's regional defaults (Brave Search in
// 13 countries, Yandex, Yahoo Japan); they assert ours now - DuckDuckGo is the
// default everywhere and Brave Search is in no list (#18, #246), while the
// regional engines are still offered. The order below the top engine is not
// asserted: sortEngines() sorts with a predicate that is not a strict weak
// order, so only "the default (or the priority engine) is first" is defined.
class InitialSearchEnginesTests: XCTestCase {

  override class func setUp() {
    super.setUp()
    Preferences.Search.shouldOverrideDSEForJapanRegion.reset()
  }

  private let everywhere: Set<SE.SearchEngineID> = [
    .google, .bing, .duckduckgo, .qwant, .startpage,
  ]

  private func assertEngines(
    _ localeSE: SE,
    are expected: Set<SE.SearchEngineID>,
    first: SE.SearchEngineID,
    priority: SE.SearchEngineID? = nil,
    file: StaticString = #filePath,
    line: UInt = #line
  ) {
    let engines = localeSE.engines.map(\.id)
    XCTAssertEqual(Set(engines), expected, file: file, line: line)
    XCTAssertEqual(engines.count, expected.count, "no engine twice", file: file, line: line)
    XCTAssertEqual(engines.first, first, file: file, line: line)
    XCTAssertEqual(Set(localeSE.onboardingEngines.map(\.id)), expected, file: file, line: line)
    XCTAssertFalse(engines.contains(.braveSearch), file: file, line: line)
    XCTAssertEqual(localeSE.defaultSearchEngine, .duckduckgo, file: file, line: line)
    XCTAssertEqual(localeSE.priorityEngine, priority, file: file, line: line)
  }

  func testDefaultValues() throws {
    let unknownLocaleSE = SE(locale: Locale(identifier: "xx_XX"))
    assertEngines(unknownLocaleSE, are: everywhere, first: .duckduckgo)
    unknownLocaleSE.engines.forEach {
      XCTAssertNil($0.customId)
    }
  }

  // MARK: - Locale overrides

  func testYandexRegionsOfferYandexWithoutMakingItTheDefault() throws {
    for region in SE().yandexDefaultRegions {
      let localeSE = SE(locale: Locale(identifier: "ru_\(region)"))
      assertEngines(localeSE, are: everywhere.union([.yandex]), first: .duckduckgo)
    }
  }

  func testNoRegionIsABraveSearchRegion() throws {
    for identifier in ["en_US", "en_GB", "de_DE", "fr_FR", "ja_JP", "pl_PL", "ru_AZ", "xx_XX"] {
      XCTAssertFalse(SE(locale: Locale(identifier: identifier)).isBraveSearchDefaultRegion)
    }
  }

  // MARK: - Country specific tests

  func testEnUS() throws {
    let localeSE = SE(locale: Locale(identifier: "en_US"))
    assertEngines(localeSE, are: everywhere.union([.ecosia]), first: .duckduckgo)
  }

  func testJaJP() throws {
    Preferences.Search.shouldOverrideDSEForJapanRegion.value = true
    defer { Preferences.Search.shouldOverrideDSEForJapanRegion.reset() }
    let localeSE = SE(locale: Locale(identifier: "ja_JP"))
    // Yahoo Japan is still the priority engine - listed first - but even on a
    // new install it is not the default any more (#247).
    assertEngines(
      localeSE,
      are: everywhere.union([.yahoojp]),
      first: .yahoojp,
      priority: .yahoojp
    )
  }

  func testEnGB() throws {
    let localeSE = SE(locale: Locale(identifier: "en_GB"))
    assertEngines(localeSE, are: everywhere.union([.ecosia]), first: .duckduckgo)
  }

  func testDeDE() throws {
    let localeSE = SE(locale: Locale(identifier: "de_DE"))
    assertEngines(localeSE, are: everywhere.union([.ecosia]), first: .duckduckgo)
  }

  func testFrFR() throws {
    let localeSE = SE(locale: Locale(identifier: "fr_FR"))
    assertEngines(localeSE, are: everywhere.union([.ecosia]), first: .duckduckgo)
  }

  func testPlPL() throws {
    let localeSE = SE(locale: Locale(identifier: "pl_PL"))
    assertEngines(localeSE, are: everywhere, first: .duckduckgo)
  }

  func testRuRu() throws {
    let localeSE = SE(locale: Locale(identifier: "ru_RU"))
    assertEngines(localeSE, are: everywhere.union([.yandex]), first: .duckduckgo)
  }

  func testKoKR() throws {
    let localeSE = SE(locale: Locale(identifier: "ko_KR"))
    assertEngines(localeSE, are: everywhere.union([.naver, .daum]), first: .duckduckgo)
  }
}
