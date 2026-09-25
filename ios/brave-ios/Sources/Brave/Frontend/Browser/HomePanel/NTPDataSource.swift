// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Preferences
import Shared
import UIKit

enum NTPWallpaper {
  case image(NTPBackgroundImage)
  // Growser-290: no sponsoredMedia - sponsored images are ads, and ads are out.

  var backgroundImage: UIImage? {
    let imagePath: URL
    switch self {
    case .image(let background):
      imagePath = background.imagePath
    }
    return UIImage(contentsOfFile: imagePath.path)
  }

  var logoImage: UIImage? {
    let imagePath: URL?
    switch self {
    case .image:
      imagePath = nil
    }
    return imagePath.flatMap { UIImage(contentsOfFile: $0.path) }
  }

  var focalPoint: CGPoint? {
    switch self {
    case .image:
      return nil  // Will eventually return a real value
    }
  }
}

public class NTPDataSource {
  // Growser-290: no rewards.

  private(set) var privateBrowsingManager: PrivateBrowsingManager

  // Data is static to avoid duplicate loads

  /// This is the number of backgrounds that must appear before a background can be repeated.
  /// So if background `3` is shown, it cannot be shown until this many backgrounds are shown, then `3` can be shown again.
  /// This does not apply to sponsored images.
  /// This is reset on each launch, so `3` can be shown again if app is removed from memory.
  /// This number _must_ be less than the number of backgrounds!
  private static let numberOfDuplicateAvoidance = 6

  let service: NTPBackgroundImagesService

  public init(
    service: NTPBackgroundImagesService,
    privateBrowsingManager: PrivateBrowsingManager
  ) {
    self.service = service
    self.privateBrowsingManager = privateBrowsingManager

    Preferences.NewTabPage.selectedCustomTheme.observe(from: self)
  }

  deinit {
    self.service.sponsoredImageDataUpdated = nil
  }

  // This is used to prevent the same handful of backgrounds from being shown.
  //  It will track the last N pictures that have been shown and prevent them from being shown
  //  until they are 'old' and dropped from this array.
  // Currently only supports normal backgrounds, as sponsored images are not supposed to be duplicate.
  // This can 'easily' be adjusted to support both sets by switching to String, and using filePath to identify uniqueness.
  private var lastBackgroundChoices = [Int]()

  // Growser-290: no shouldAttemptSponsoredMedia() or getSponsoredMediaBackground(for:).

  func getImageBackground() -> NTPWallpaper? {
    // Identifying the background array to use
    let backgroundSet = {
      () -> [NTPWallpaper] in

      if service.backgroundImages.isEmpty {
        return [NTPWallpaper.image(.fallback)]
      }
      return service.backgroundImages.map(NTPWallpaper.image)
    }()

    if backgroundSet.isEmpty { return nil }

    // Choosing the actual index / item to use
    let backgroundIndex = { () -> Int in
      let availableRange = 0..<backgroundSet.count
      // This takes all indeces and filters out ones that were shown recently
      let availableBackgroundIndeces = availableRange.filter {
        !self.lastBackgroundChoices.contains($0)
      }
      // Due to how many display modes currently exist, the background avoidance counter may get utilized on a smaller subset.
      // This can be repro by swapping between normal backgrounds and a super referrer, where all available indeces get squeezed out, resulting in an empty set.
      // To avoid issues, first fallback results in full set.

      // Chooses a new random index to use from the available indeces
      let chosenIndex =
        availableBackgroundIndeces.randomElement() ?? availableRange.randomElement() ?? -1
      assert(chosenIndex >= 0, "NTP index was nil, this is terrible.")
      assert(chosenIndex < backgroundSet.count, "NTP index is too large, BAD!")

      // This index is now added to 'past' tracking list to prevent duplicates
      self.lastBackgroundChoices.append(chosenIndex)
      // Trimming to fixed length to release older backgrounds

      self.lastBackgroundChoices = self.lastBackgroundChoices
        .suffix(min(backgroundSet.count - 1, NTPDataSource.numberOfDuplicateAvoidance))
      return chosenIndex
    }()

    return backgroundSet[safe: backgroundIndex]
  }

  func newBackground(completion: @escaping (NTPWallpaper?) -> Void) {
    if !Preferences.NewTabPage.backgroundImages.value { return completion(nil) }

    // Force back to `0` if at end
    Preferences.NewTabPage.backgroundRotationCounter.value %= service.countToBrandedWallpaper
    // Increment regardless, this is a counter, not an index, so smallest should be `1`
    Preferences.NewTabPage.backgroundRotationCounter.value += 1

    // Growser-290: no sponsored image to try first - ads are out.
    completion(getImageBackground())
  }
}

extension NTPDataSource: PreferencesObserver {
  public func preferencesDidChange(for key: String) {
    let customThemePref = Preferences.NewTabPage.selectedCustomTheme
    let installedThemesPref = Preferences.NewTabPage.installedCustomThemes

    switch key {
    case customThemePref.key:
      let installedThemes = installedThemesPref.value
      if let theme = customThemePref.value, !installedThemes.contains(theme) {
        installedThemesPref.value = installedThemesPref.value + [theme]
      }
    default:
      break
    }
  }
}

extension NTPBackgroundImage {
  // Growser-310: the owner's photo, credited and linked the way Android does
  // (#309). The component that would bring others is not served to a fork.
  static let fallback: NTPBackgroundImage = .init(
    imagePath: Bundle.module.url(forResource: "growser_mobile_01", withExtension: "webp")!,
    author: "Dmitry Golubnichiy",
    link: URL(string: "https://www.flickr.com/photos/dsfox/7492378116")!
  )
}
