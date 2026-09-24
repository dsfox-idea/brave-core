// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Data
import Foundation
import Preferences
import Shared
import UIKit

/// growser#310: the sites the person visits most, as tiles on the new tab
/// page - history ranked by frecency (Chromium's `ntp_tiles`, through
/// `MostVisitedSites`), which is what Android's new tab page shows (#305).
///
/// It sits under the favourites and looks like them. A site already among the
/// favourites is not shown twice, and nothing is shown in private browsing.
/// The favourites switch in the new tab page settings hides both: to a person
/// they are one grid of sites.
class MostVisitedSectionProvider: NSObject, NTPObservableSectionProvider, MostVisitedSitesObserver {
  var sectionDidChange: (() -> Void)?

  /// Two rows at most, like Android's grid of eight.
  static let rows = 2

  private let mostVisited: (any MostVisitedSites)?
  private var observation: (any MostVisitedSitesScopedObservation)?
  private let open: (_ url: URL, _ inNewTab: Bool) -> Void
  private var tiles: [NTPTile] = []
  /// What the grid shows, fixed when the section is counted.
  private var shown: [NTPTile] = []

  init(
    profile: any Profile,
    isPrivateBrowsing: Bool,
    open: @escaping (_ url: URL, _ inNewTab: Bool) -> Void
  ) {
    self.open = open
    mostVisited = isPrivateBrowsing ? nil : MostVisitedSitesFactory.get(for: profile)
    super.init()
    observation = mostVisited?.addMostVisitedURLsObserver(self, maxNumSites: 20)
    mostVisited?.refresh()
  }

  // MARK: - MostVisitedSitesObserver

  func mostVisitedSitesDidUpdateTiles(_ tiles: [NTPTile]) {
    DispatchQueue.main.async {
      self.tiles = tiles
      self.sectionDidChange?()
    }
  }

  func mostVisitedSitesDidUpdateFavicon(for url: URL?) {}

  // MARK: - NTPSectionProvider

  func registerCells(to collectionView: UICollectionView) {
    collectionView.register(FavoritesCell.self, forCellWithReuseIdentifier: FavoritesCell.identifier)
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    guard Preferences.NewTabPage.showNewTabFavourites.value else {
      shown = []
      return 0
    }
    let columns = FavoritesSectionProvider.numberOfItems(
      in: collectionView,
      availableWidth: fittingSizeForCollectionView(collectionView, section: section).width
    )
    shown = Self.visible(
      tiles,
      favourites: Set(Favorite.allFavorites.compactMap(\.url)),
      capacity: columns * Self.rows
    )
    return shown.count
  }

  /// The tiles the grid shows: in the order the ranking gives, none that is
  /// already a favourite, and no more than fit.
  static func visible(_ tiles: [NTPTile], favourites: Set<String>, capacity: Int) -> [NTPTile] {
    Array(tiles.filter { !favourites.contains($0.url.absoluteString) }.prefix(max(capacity, 0)))
  }

  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    collectionView.dequeueReusableCell(withReuseIdentifier: FavoritesCell.identifier, for: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    willDisplay cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    guard let cell = cell as? FavoritesCell, let tile = shown[safe: indexPath.item] else { return }
    cell.title = tile.title.isEmpty ? tile.url.host() ?? tile.url.absoluteString : tile.title
    cell.imageView.cancelLoading()
    cell.imageView.loadFavicon(siteURL: tile.url, isPrivateBrowsing: false)
    cell.accessibilityLabel = cell.title
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let tile = shown[safe: indexPath.item] else { return }
    open(tile.url, false)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let indexPath = indexPaths.first, let tile = shown[safe: indexPath.item] else {
      return nil
    }
    return UIContextMenuConfiguration(identifier: indexPath as NSCopying, previewProvider: nil) {
      _ in
      // No "open in a private tab": this path does not ask for the private
      // browsing lock the way the favourites' does.
      let openInNewTab = UIAction(
        title: Strings.openNewTabButtonTitle,
        handler: UIAction.deferredActionHandler { _ in self.open(tile.url, true) }
      )
      let hide = UIAction(
        title: Strings.recentSearchHide,
        attributes: .destructive,
        handler: UIAction.deferredActionHandler { _ in
          self.mostVisited?.setBlocked(true, for: tile.url)
        }
      )
      return UIMenu(title: tile.title, children: [openInNewTab, hide])
    }
  }

  // MARK: - Layout, the favourites' own

  private func itemSize(_ collectionView: UICollectionView, section: Int) -> CGSize {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    let columns = FavoritesSectionProvider.numberOfItems(in: collectionView, availableWidth: width)
    if floor(width / CGFloat(columns)) < FavoritesSectionProvider.defaultIconSize.width {
      let side = floor(width / 4.0)
      return CGSize(width: side, height: FavoritesCell.height(forWidth: side))
    }
    return FavoritesSectionProvider.defaultIconSize
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    itemSize(collectionView, section: indexPath.section)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    let insets = horizontalInsets(
      for: collectionView,
      maxWidth: FavoritesSectionProvider.maxWidth,
      minimumInset: 16
    )
    return UIEdgeInsets(top: 8, left: insets.left, bottom: 8, right: insets.right)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumInteritemSpacingForSectionAt section: Int
  ) -> CGFloat {
    let width = fittingSizeForCollectionView(collectionView, section: section).width
    let columns = FavoritesSectionProvider.numberOfItems(in: collectionView, availableWidth: width)
    let size = itemSize(collectionView, section: section)
    return floor((width - size.width * CGFloat(columns)) / (CGFloat(columns) - 1))
  }
}
