// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import CryptoKit
import Foundation
import Shared
import os.log

/// growser#297: the catalogue's filter lists, fetched from their publishers.
///
/// A catalogue entry names a component, and Brave's component server does not
/// serve a fork, so on iOS the lists never arrived: the engine had none and the
/// Shields count read 0. The entry also names the publishers its list is built
/// from (`sourceURLs`), which need no key - the desktop fetches them the same
/// way (#87). Every source of an enabled entry is downloaded, the sources are
/// joined into one file in their catalogue order, and that file is handed to the
/// engine under the entry's own source, exactly as the component would have
/// delivered it. All sources, not the first: the component packs them together,
/// and one of eight would quietly block less than the list promises.
@MainActor final class FilterListPublisherDownloader {
  static let shared = FilterListPublisherDownloader()

  private let downloader = ResourceDownloader<FilterListPublisherSource>()
  /// One refresh loop per enabled entry, keyed by the entry's uuid.
  private var tasks: [String: Task<Void, Never>] = [:]
  private var subscription: AnyCancellable?
  private var resourcesTask: Task<Void, Never>?

  /// The scriptlets and redirect resources that `+js(...)` rules name. This
  /// file is the whole of Brave's resources component - the universal uBO
  /// scriptlets are compiled into adblock-rust itself (measured for the
  /// desktop in #87) - so it replaces the component, which a fork is not served.
  static let resourcesURL = URL(
    string: "https://raw.githubusercontent.com/brave/adblock-resources/master/dist/resources.json"
  )!

  private static var fetchInterval: TimeInterval {
    AppConstants.isOfficialBuild ? 6.hours : 10.minutes
  }

  /// Follow the catalogue: fetch what is enabled, stop what is not.
  func start() {
    guard subscription == nil else { return }
    resourcesTask = Task { [downloader] in
      while !Task.isCancelled {
        await Self.publishResources(downloader: downloader)
        try? await Task.sleep(for: .seconds(Self.fetchInterval))
      }
    }
    subscription = FilterListStorage.shared.$filterLists.sink { lists in
      Task { @MainActor [weak self] in self?.follow(lists) }
    }
  }

  private func follow(_ lists: [FilterList]) {
    var enabled: [String: FilterList] = [:]
    for list in lists where list.isEnabled && !list.entry.sourceURLs.isEmpty {
      enabled[list.id] = list
    }
    for (id, task) in tasks where enabled[id] == nil {
      task.cancel()
      tasks[id] = nil
    }
    for (id, list) in enabled where tasks[id] == nil {
      tasks[id] = refreshLoop(for: list.entry)
    }
    ContentBlockerManager.log.debug(
      "growser#297: \(enabled.count, privacy: .public) catalogue lists from their publishers"
    )
  }

  private func refreshLoop(for entry: AdblockFilterListCatalogEntry) -> Task<Void, Never> {
    let sources = entry.sourceURLs.compactMap(URL.init(string:)).map {
      FilterListPublisherSource(entryId: entry.uuid, url: $0)
    }
    return Task { [downloader] in
      while !Task.isCancelled {
        await Self.publish(entry: entry, sources: sources, downloader: downloader)
        try? await Task.sleep(for: .seconds(Self.fetchInterval))
      }
    }
  }

  private static func publish(
    entry: AdblockFilterListCatalogEntry,
    sources: [FilterListPublisherSource],
    downloader: ResourceDownloader<FilterListPublisherSource>
  ) async {
    var parts: [String] = []
    var newest = Date.distantPast
    for source in sources {
      do {
        let result = try await downloader.download(resource: source)
        parts.append(try String(contentsOf: result.fileURL, encoding: .utf8))
        newest = max(newest, result.date)
      } catch {
        // A publisher that is down keeps its last good copy rather than
        // shrinking the list; one never fetched is missing until it answers.
        if let cached = source.downloadedFileURL,
          let text = try? String(contentsOf: cached, encoding: .utf8)
        {
          parts.append(text)
        }
        ContentBlockerManager.log.error(
          "growser#297: \(source.url.absoluteString, privacy: .public): \(error.localizedDescription, privacy: .public)"
        )
      }
    }
    guard !parts.isEmpty else { return }

    do {
      let folder = try FileManager.default.url(
        for: .cachesDirectory,
        in: .userDomainMask,
        appropriateFor: nil,
        create: true
      ).appending(path: "filter-list-publishers/\(entry.uuid)", directoryHint: .isDirectory)
      try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
      let listURL = folder.appending(path: "list.txt")
      try parts.joined(separator: "\n").write(to: listURL, atomically: true, encoding: .utf8)

      let fileInfo = AdBlockEngineManager.FileInfo(
        filterListInfo: GroupedAdBlockEngine.FilterListInfo(
          source: entry.engineSource,
          version: String(Int(newest.timeIntervalSince1970))
        ),
        localFileURL: listURL
      )
      AdBlockGroupsManager.shared.update(fileInfo: fileInfo)
      AdBlockGroupsManager.shared.compileEnginesIfFilesAreReady()
      ContentBlockerManager.log.debug(
        "growser#297: \(entry.title, privacy: .public) - \(parts.count, privacy: .public)/\(sources.count, privacy: .public) sources"
      )
    } catch {
      ContentBlockerManager.log.error(
        "growser#297: \(entry.title, privacy: .public): \(error.localizedDescription, privacy: .public)"
      )
    }
  }
}

extension FilterListPublisherDownloader {
  /// Hand the engines the publisher's resources. The engine versions its
  /// resources by the name of the folder the file is in, so each copy goes
  /// into a folder named by the time the publisher last changed it.
  fileprivate static func publishResources(
    downloader: ResourceDownloader<FilterListPublisherSource>
  ) async {
    let source = FilterListPublisherSource(entryId: "resources", url: resourcesURL)
    do {
      let result = try await downloader.download(resource: source)
      let version = String(Int(result.date.timeIntervalSince1970))
      let folder = try FileManager.default.url(
        for: .cachesDirectory,
        in: .userDomainMask,
        appropriateFor: nil,
        create: true
      ).appending(path: "filter-list-publishers/resources/\(version)", directoryHint: .isDirectory)
      let fileURL = folder.appending(path: "resources.json")
      if !FileManager.default.fileExists(atPath: fileURL.path(percentEncoded: false)) {
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
        try FileManager.default.copyItem(at: result.fileURL, to: fileURL)
      }
      await AdBlockGroupsManager.shared.didDownloadResources(fileURL: fileURL)
      ContentBlockerManager.log.debug(
        "growser#297: resources \(version, privacy: .public) from their publisher"
      )
    } catch {
      ContentBlockerManager.log.error(
        "growser#297: resources: \(error.localizedDescription, privacy: .public)"
      )
    }
  }
}

/// One publisher URL of one catalogue entry, cached under that entry.
struct FilterListPublisherSource: DownloadResourceInterface, Hashable {
  let entryId: String
  let url: URL

  var cacheFolderName: String { "filter-list-publishers/\(entryId)/sources" }
  /// Stable per URL, so a list keeps its etag and its last good copy.
  var cacheFileName: String {
    SHA256.hash(data: Data(url.absoluteString.utf8)).map { String(format: "%02x", $0) }.joined()
  }
  var externalURL: URL { url }
  var headers: [String: String] { [:] }
}
