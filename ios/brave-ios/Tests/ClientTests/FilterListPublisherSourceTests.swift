// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import XCTest

@testable import Brave

/// growser#297: a publisher source is cached under its catalogue entry, one
/// file per URL, and the name must not move - it is what keeps a source's
/// etag and its last good copy, which is what a publisher that is down falls
/// back to.
class FilterListPublisherSourceTests: XCTestCase {
  private let easyList = URL(string: "https://easylist.to/easylist/easylist.txt")!
  private let easyPrivacy = URL(string: "https://easylist.to/easylist/easyprivacy.txt")!

  func testTheSameURLAlwaysGetsTheSameFile() {
    let first = FilterListPublisherSource(entryId: "default", url: easyList)
    let again = FilterListPublisherSource(entryId: "default", url: easyList)
    XCTAssertEqual(first.cacheFileName, again.cacheFileName)
    XCTAssertEqual(first.cacheFileName.count, 64, "a SHA-256 in hex")
  }

  func testTwoSourcesOfOneEntryDoNotShareAFile() {
    let ads = FilterListPublisherSource(entryId: "default", url: easyList)
    let privacy = FilterListPublisherSource(entryId: "default", url: easyPrivacy)
    XCTAssertNotEqual(ads.cacheFileName, privacy.cacheFileName)
    XCTAssertEqual(ads.cacheFolderName, privacy.cacheFolderName)
  }

  func testEachEntryHasItsOwnFolder() {
    let inDefault = FilterListPublisherSource(entryId: "default", url: easyList)
    let inAnother = FilterListPublisherSource(entryId: "other", url: easyList)
    XCTAssertNotEqual(inDefault.cacheFolderName, inAnother.cacheFolderName)
    XCTAssertTrue(inDefault.cacheFolderName.contains("default"))
  }

  func testTheRequestIsThePublishersURLWithNoKey() {
    let source = FilterListPublisherSource(entryId: "default", url: easyList)
    XCTAssertEqual(source.externalURL, easyList)
    XCTAssertTrue(source.headers.isEmpty)
  }
}
