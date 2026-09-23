// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
// Growser-281: no BraveNews.
import BraveShared
import BraveUI
import Combine
import CoreData
import Data
import DesignSystem
import Growth
import Preferences
import Shared
import SnapKit
import SwiftUI
import UIKit
import Web

/// A section that will be shown in the NTP. Sections are responsible for the
/// layout and interaction of their own items
protocol NTPSectionProvider: NSObject, UICollectionViewDelegateFlowLayout,
  UICollectionViewDataSource
{
  /// Register cells and supplimentary views for your section to
  /// `collectionView`
  func registerCells(to collectionView: UICollectionView)
}

extension NTPSectionProvider {
  /// The bounding size for auto-sizing cells, bound to the maximum available
  /// width in the collection view, taking into account safe area insets and
  /// insets for that given section
  func fittingSizeForCollectionView(_ collectionView: UICollectionView, section: Int) -> CGSize {
    let sectionInset: UIEdgeInsets
    if let flowLayout = collectionView.collectionViewLayout as? UICollectionViewFlowLayout {
      if let flowLayoutDelegate = collectionView.delegate as? UICollectionViewDelegateFlowLayout {
        sectionInset =
          flowLayoutDelegate.collectionView?(
            collectionView,
            layout: collectionView.collectionViewLayout,
            insetForSectionAt: section
          ) ?? flowLayout.sectionInset
      } else {
        sectionInset = flowLayout.sectionInset
      }
    } else {
      sectionInset = .zero
    }
    return CGSize(
      width: max(
        0,
        collectionView.bounds.width - collectionView.safeAreaInsets.left
          - collectionView.safeAreaInsets.right - sectionInset.left - sectionInset.right
      ),
      height: 1000
    )
  }

  /// Horizontal section insets that constrain content to a maximum width,
  /// centering it within the available space. When vertical space is limited
  /// (iPhone landscape, or any device below `compactHeightThreshold`) the
  /// content is instead pinned to the trailing half of the collection view so
  /// the leading side stays clear for the sponsored image logo button.
  ///
  /// `minimumInset` is the smallest allowed horizontal inset (e.g. 16pt).
  func horizontalInsets(
    for collectionView: UICollectionView,
    maxWidth: CGFloat,
    minimumInset: CGFloat
  ) -> (left: CGFloat, right: CGFloat) {
    /// The available height below which content is pinned to the trailing half
    /// so the sponsored image logo button remains tappable.
    let compactHeightThreshold: CGFloat = 500
    let compactWidthThreshold: CGFloat = 580
    let availableWidth =
      collectionView.bounds.width - collectionView.safeAreaInsets.left
      - collectionView.safeAreaInsets.right
    let availableHeight =
      collectionView.bounds.height - collectionView.safeAreaInsets.top
      - collectionView.safeAreaInsets.bottom
    let isLandscape = collectionView.bounds.width > collectionView.bounds.height
    let isCompactHeight = availableHeight < compactHeightThreshold
    let isCompactWidth = availableWidth < compactWidthThreshold
    if (UIDevice.isPhone && isLandscape) || (isCompactHeight && !isCompactWidth) {
      // Pin the content to the trailing half of the collection view, centering
      // it within that half (capped at `maxWidth`).
      let halfWidth = availableWidth / 2.0
      let contentWidth = min(halfWidth - minimumInset * 2, maxWidth)
      let gap = max(minimumInset, (halfWidth - contentWidth) / 2)
      return (left: halfWidth + gap, right: gap)
    }
    let contentWidth = min(availableWidth - minimumInset * 2, maxWidth)
    let inset = max(minimumInset, (availableWidth - contentWidth) / 2)
    return (inset, inset)
  }
}

/// A section provider that can be observed for changes to tell the
/// `NewTabPageViewController` to reload its section
protocol NTPObservableSectionProvider: NTPSectionProvider {
  var sectionDidChange: (() -> Void)? { get set }
}

protocol NewTabPageDelegate: AnyObject {
  func focusURLBar()
  func navigateToInput(_ input: String, inNewTab: Bool, switchingToPrivateMode: Bool)
  func handleFavoriteAction(favorite: Favorite, action: BookmarksAction)
  // Growser-290: no brandedImageCalloutActioned(_:) or
  // showNewTabTakeoverInfoBarIfNeeded() - both were sponsored images'.
  func showNTPOnboarding()
  func isNewTabPageOccluded() -> Bool
}

/// The new tab page. Shows users a variety of information, including stats and
/// favourites
class NewTabPageViewController: UIViewController {
  weak var delegate: NewTabPageDelegate?

  var ntpStatsOnboardingFrame: CGRect? {
    guard let section = sections.firstIndex(where: { $0 is StatsSectionProvider }) else {
      return nil
    }

    if let cell = collectionView.cellForItem(at: IndexPath(item: 0, section: section)) {
      return cell.contentView.convert(cell.contentView.frame, to: view)
    }
    return nil
  }

  /// The modules to show on the new tab page
  private var sections: [NTPSectionProvider] = []

  private let layout = NewTabPageFlowLayout()
  private let collectionView: NewTabCollectionView
  private weak var browserTab: (any TabState)?
  // Growser-290: no rewards.

  private var background: NewTabPageBackground
  private let backgroundView = NewTabPageBackgroundView()
  private let backgroundButtonsView: NewTabPageBackgroundButtonsView

  /// A gradient to display over background images to ensure visibility of
  /// the NTP contents and sponsored logo
  ///
  /// Only should be displayed when the user has background images enabled
  let gradientView = GradientView(
    colors: [
      UIColor(white: 0.0, alpha: 0.5),
      UIColor(white: 0.0, alpha: 0.0),
      UIColor(white: 0.0, alpha: 0.3),
    ],
    positions: [0, 0.5, 0.8],
    startPoint: .zero,
    endPoint: CGPoint(x: 0, y: 1)
  )

  // Growser-281: no feed data source, feed overlay or news reload guard.

  // Growser-290: no branded-image notifications - sponsored images are ads.
  private var cancellables: Set<AnyCancellable> = []
  private let privateBrowsingManager: PrivateBrowsingManager

  private let profilePrefs: any PrefService

  init(
    tab: some TabState,
    profilePrefs: any PrefService,
    dataSource: NTPDataSource,
    // Growser-281: no feedDataSource. Growser-290: no rewards.
    privateBrowsingManager: PrivateBrowsingManager
  ) {
    self.browserTab = tab
    self.profilePrefs = profilePrefs
    self.privateBrowsingManager = privateBrowsingManager
    self.backgroundButtonsView = NewTabPageBackgroundButtonsView(
      privateBrowsingManager: privateBrowsingManager,
      profilePrefs: profilePrefs
    )
    background = NewTabPageBackground(dataSource: dataSource)
    collectionView = NewTabCollectionView(frame: .zero, collectionViewLayout: layout)
    super.init(nibName: nil, bundle: nil)

    Preferences.NewTabPage.showNewTabPrivacyHub.observe(from: self)
    Preferences.NewTabPage.showNewTabFavourites.observe(from: self)

    sections = [
      StatsSectionProvider(
        isPrivateBrowsing: tab.isPrivate,
        openPrivacyHubPressed: { [weak self] in
          guard let self, let tab = browserTab else { return }
          if privateBrowsingManager.isPrivateBrowsing == true {
            return
          }

          let isOriginPurchased =
            BraveOriginServiceFactory.get(profile: tab.profile)?.isPurchased() == true
          let host = UIHostingController(
            rootView: PrivacyReportsManager.prepareView(
              isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing,
              isOriginPurchased: isOriginPurchased
            )
          )
          host.rootView.onDismiss = { [weak self] in
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
              guard let self = self else { return }

              // Handle App Rating
              // User finished viewing the privacy report (tapped close)
              AppReviewManager.shared.handleAppReview(for: .revised, using: self)
            }
          }

          host.rootView.openPrivacyReportsUrl = { [weak self] in
            self?.delegate?.navigateToInput(
              URL.brave.privacyFeatures.absoluteString,
              inNewTab: false,
              // Privacy Reports view is unavailable in private mode.
              switchingToPrivateMode: false
            )
          }

          present(host, animated: true)
        },
        hidePrivacyHubPressed: { [weak self] in
          self?.hidePrivacyHub()
        }
      ),
      FavoritesSectionProvider(
        action: { [weak self] bookmark, action in
          self?.handleFavoriteAction(favorite: bookmark, action: action)
        },
        legacyLongPressAction: { [weak self] alertController in
          self?.present(alertController, animated: true)
        },
        isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
      ),
      FavoritesOverflowSectionProvider(action: { [weak self] in
        self?.delegate?.focusURLBar()
      }),
    ]

    let ntpDefaultBrowserCalloutProvider = NTPDefaultBrowserCalloutProvider(
      isBackgroundNTPSI: false  // Growser-290: there are no sponsored images.
    )

    // This is a one-off view, adding it to the NTP only if necessary.
    if ntpDefaultBrowserCalloutProvider.shouldShowCallout() {
      sections.insert(ntpDefaultBrowserCalloutProvider, at: 0)
    }

    // Growser-281: no Brave News section.

    collectionView.do {
      $0.delegate = self
      $0.dataSource = self
      $0.dragDelegate = self
      $0.dropDelegate = self
    }

    background.changed = { [weak self] in
      guard let self else { return }
      setupBackgroundImage()
      // Growser-290: no sponsored background impression to report.
    }

    // Growser-281: no Brave News observers or usage P3A.

    recordNewTabCreatedP3A()
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    view.addSubview(backgroundView)
    view.insertSubview(gradientView, aboveSubview: backgroundView)
    view.addSubview(collectionView)
    // Growser-281: no feed overlay.

    collectionView.backgroundView = backgroundButtonsView

    backgroundButtonsView.tappedActiveButton = { [weak self] sender in
      self?.tappedActiveBackgroundButton(sender)
    }

    setupBackgroundImage()
    backgroundView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }
    collectionView.snp.makeConstraints {
      $0.edges.equalToSuperview()
    }

    gradientView.snp.makeConstraints {
      $0.edges.equalTo(backgroundView)
    }

    sections.enumerated().forEach { (index, provider) in
      provider.registerCells(to: collectionView)
      if let observableProvider = provider as? NTPObservableSectionProvider {
        observableProvider.sectionDidChange = { [weak self] in
          guard let self = self else { return }
          if self.parent != nil {
            UIView.performWithoutAnimation {
              // As of iOS 16.4, reloadSections seems to do some sort of validation of the underlying data
              // for other sections that aren't being refreshed. This can cause assertions for sections that
              // may need to reload in the same batch but don't. Since we don't animate this section anyways
              // we can just switch to `reloadData` here.
              self.collectionView.reloadData()
            }
          }
          self.collectionView.collectionViewLayout.invalidateLayout()
        }
      }
    }

    registerForTraitChanges([UITraitVerticalSizeClass.self]) { (self: Self, _) in
      self.calculateBackgroundCenterPoints()
    }
    registerForTraitChanges([UITraitHorizontalSizeClass.self]) { (self: Self, _) in
      self.collectionView.reloadData()
    }
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    collectionView.reloadData()
    // Growser-281: no feed update check.
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()

    collectionView.collectionViewLayout.invalidateLayout()

    // Make sure that imageView has a frame calculated before we attempt
    // to use it.
    backgroundView.layoutIfNeeded()

    calculateBackgroundCenterPoints()
  }

  override func viewWillTransition(
    to size: CGSize,
    with coordinator: any UIViewControllerTransitionCoordinator
  ) {
    super.viewWillTransition(to: size, with: coordinator)
    guard
      let favoriteSection = sections.firstIndex(where: { $0 is FavoritesSectionProvider }),
      let provider = sections[favoriteSection] as? FavoritesSectionProvider
    else {
      return
    }
    // Only reload the favorites section (and its overflow section) when the
    // number of favorites actually displayed would change, otherwise favorites
    // may wrap onto a second row. The available width isn't known until the
    // collection view's bounds & insets update, so compute the new displayed
    // count in the transition completion handler.
    let currentCount = collectionView.numberOfItems(inSection: favoriteSection)
    coordinator.animate(alongsideTransition: nil) { [weak self] _ in
      guard let self else { return }
      let updatedCount = provider.displayedItemCount(
        in: self.collectionView,
        section: favoriteSection
      )
      if currentCount != updatedCount {
        self.collectionView.reloadSections(IndexSet([favoriteSection, favoriteSection + 1]))
      }
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    // Growser-290: no sponsored background impression or branded-image
    // notification.

    DispatchQueue.main.asyncAfter(deadline: .now() + 0.50) {
      self.delegate?.showNTPOnboarding()
    }
  }

  override func viewSafeAreaInsetsDidChange() {
    super.viewSafeAreaInsetsDidChange()

    backgroundButtonsView.collectionViewSafeAreaInsets = view.safeAreaInsets
  }

  override func willMove(toParent parent: UIViewController?) {
    super.willMove(toParent: parent)

    backgroundView.imageView.image = parent == nil ? nil : background.backgroundImage
  }

  // MARK: - Background

  // Growser-290: no hideVisibleSponsoredImageNotification().

  func setupBackgroundImage() {
    collectionView.reloadData()

    if let background = background.currentBackground {
      switch background {
      case .image(let background):
        if case let name = background.author, !name.isEmpty {
          backgroundButtonsView.activeButton = .imageCredit(name)
        } else {
          backgroundButtonsView.activeButton = .none
        }
      }
    } else {
      backgroundButtonsView.activeButton = .none
    }

    gradientView.isHidden = background.backgroundImage == nil
    backgroundView.imageView.image = background.backgroundImage
  }

  private func calculateBackgroundCenterPoints() {

    // Only iPhone portrait looking devices have their center of the image offset adjusted.
    // In other cases the image is always centered.
    guard let image = backgroundView.imageView.image,
      traitCollection.horizontalSizeClass == .compact
        && traitCollection.verticalSizeClass == .regular
    else {
      // Reset the previously calculated offset.
      backgroundView.updateImageXOffset(by: 0)
      return
    }

    // If no focal point provided we do nothing. The image is centered by default.
    guard let focalPoint = background.currentBackground?.focalPoint else {
      return
    }

    let focalX = focalPoint.x

    // Calculate the sizing difference between `image` and `imageView` to determine the pixel difference ratio.
    // Most image calculations have to use this property to get coordinates right.
    let sizeRatio = backgroundView.imageView.frame.size.height / image.size.height

    // How much the image should be offset according to the set focal point coordinate.
    // We calculate it by looking how much to move the image away from the center of the image.
    let focalXOffset = ((image.size.width / 2) - focalX) * sizeRatio

    // Amount of image space which is cropped on one side, not visible on the screen.
    // We use this info to prevent going of out image bounds when updating the `x` offset.
    let extraHorizontalSpaceOnOneSide =
      ((image.size.width * sizeRatio) - backgroundView.frame.width) / 2

    // The offset proposed by the focal point might be too far away from image's center
    // resulting in not having anough image space to cover entire width of the view and leaving blank space.
    // If the focal offset goes out of bounds we center it to the maximum amount we can where the entire
    // image is able to cover the view.
    let realisticXOffset =
      abs(focalXOffset) > extraHorizontalSpaceOnOneSide
      ? extraHorizontalSpaceOnOneSide : focalXOffset

    backgroundView.updateImageXOffset(by: realisticXOffset)
  }

  // MARK: - Sponsored background events

  // Growser-290: no sponsored background events - ads are out.

  // Growser-290: no branded-image notifications.

  // Growser-281: no Brave News actions, feed state handling or loading.

  private func hidePrivacyHub() {
    if Preferences.NewTabPage.hidePrivacyHubAlertShown.value {
      Preferences.NewTabPage.showNewTabPrivacyHub.value = false
      collectionView.reloadData()
    } else {
      let alert = UIAlertController(
        title: Strings.PrivacyHub.hidePrivacyHubWidgetActionTitle,
        message: Strings.PrivacyHub.hidePrivacyHubWidgetAlertDescription,
        preferredStyle: .alert
      )

      alert.addAction(UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel))
      alert.addAction(
        UIAlertAction(
          title: Strings.PrivacyHub.hidePrivacyHubWidgetActionButtonTitle,
          style: .default
        ) { [weak self] _ in
          Preferences.NewTabPage.showNewTabPrivacyHub.value = false
          Preferences.NewTabPage.hidePrivacyHubAlertShown.value = true
          self?.collectionView.reloadData()
        }
      )

      UIImpactFeedbackGenerator(style: .medium).vibrate()
      present(alert, animated: true, completion: nil)
    }
  }

  // MARK: - Actions

  // Growser-281: no tappedNewContentAvailable() or tappedBraveNewsSettings().

  private func tappedActiveBackgroundButton(_ sender: UIControl) {
    guard let background = background.currentBackground else { return }
    switch background {
    case .image:
      presentImageCredit(sender)
    }
  }

  // Growser-290: no tappedSponsorButton(_:).

  private func handleFavoriteAction(favorite: Favorite, action: BookmarksAction) {
    delegate?.handleFavoriteAction(favorite: favorite, action: action)
  }

  private func presentImageCredit(_ button: UIControl) {
    guard case .image(let background) = background.currentBackground else { return }

    let alert = UIAlertController(
      title: background.author,
      message: nil,
      preferredStyle: .actionSheet
    )

    if let creditURL = background.link {
      let websiteTitle = String(format: Strings.viewOn, creditURL.hostSLD.capitalizeFirstLetter)
      alert.addAction(
        UIAlertAction(title: websiteTitle, style: .default) { [weak self] _ in
          self?.delegate?.navigateToInput(
            creditURL.absoluteString,
            inNewTab: false,
            switchingToPrivateMode: false
          )
        }
      )
    }

    alert.popoverPresentationController?.sourceView = button
    alert.popoverPresentationController?.permittedArrowDirections = [.down, .up]
    alert.addAction(UIAlertAction(title: Strings.close, style: .cancel, handler: nil))

    UIImpactFeedbackGenerator(style: .medium).vibrate()
    present(alert, animated: true, completion: nil)
  }

  // Growser-281: no longPressedBraveNewsSettingsButton().
}

extension NewTabPageViewController: PreferencesObserver {
  func preferencesDidChange(for key: String) {
    if key == Preferences.NewTabPage.showNewTabPrivacyHub.key
      || key == Preferences.NewTabPage.showNewTabFavourites.key
    {
      collectionView.reloadData()
      return
    }

    // Growser-281: the only other observed key was Brave News'.
  }
}

// MARK: - UIScrollViewDelegate
extension NewTabPageViewController {
  // Growser-281: no isBraveNewsVisible.

  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    for section in sections {
      section.scrollViewDidScroll?(scrollView)
    }
    // Growser-281: no Brave News scroll behaviour.
  }

  // Growser-281: no scrollToBraveNews().

  // MARK: - P3A

  // Growser-281: no Brave News usage P3A.

  private func recordNewTabCreatedP3A() {
    var newTabsStorage = P3ATimedStorage<Int>.newTabsCreatedStorage
    var sponsoredStorage = P3ATimedStorage<Int>.sponsoredNewTabsCreatedStorage

    newTabsStorage.add(value: 1, to: Date())
    let newTabsCreatedAnswer = newTabsStorage.maximumDaysCombinedValue

    // Growser-290: no sponsored new tabs to count.

    UmaHistogramRecordValueToBucket(
      "Brave.NTP.NewTabsCreated.3",
      buckets: [
        0,
        1,
        2,
        3,
        4,
        .r(5...8),
        .r(9...15),
        .r(16...),
      ],
      value: newTabsCreatedAnswer
    )

    if newTabsCreatedAnswer > 0 {
      let sponsoredPercent = Int(
        (Double(sponsoredStorage.maximumDaysCombinedValue) / Double(newTabsCreatedAnswer)) * 100.0
      )
      UmaHistogramRecordValueToBucket(
        "Brave.NTP.SponsoredNewTabsCreated",
        buckets: [
          0,
          .r(0..<10),
          .r(10..<20),
          .r(20..<30),
          .r(30..<40),
          .r(40..<50),
          .r(50...),
        ],
        value: sponsoredPercent
      )
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension NewTabPageViewController: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    sections[indexPath.section].collectionView?(collectionView, didSelectItemAt: indexPath)
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    sizeForItemAt indexPath: IndexPath
  ) -> CGSize {
    sections[indexPath.section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      sizeForItemAt: indexPath
    ) ?? .zero
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    insetForSectionAt section: Int
  ) -> UIEdgeInsets {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      insetForSectionAt: section
    ) ?? .zero
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumLineSpacingForSectionAt section: Int
  ) -> CGFloat {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      minimumLineSpacingForSectionAt: section
    ) ?? 0
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    minimumInteritemSpacingForSectionAt section: Int
  ) -> CGFloat {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      minimumInteritemSpacingForSectionAt: section
    ) ?? 0
  }
  func collectionView(
    _ collectionView: UICollectionView,
    layout collectionViewLayout: UICollectionViewLayout,
    referenceSizeForHeaderInSection section: Int
  ) -> CGSize {
    sections[section].collectionView?(
      collectionView,
      layout: collectionViewLayout,
      referenceSizeForHeaderInSection: section
    ) ?? .zero
  }
}

// MARK: - UICollectionViewDelegate
extension NewTabPageViewController: UICollectionViewDelegate {
  func collectionView(
    _ collectionView: UICollectionView,
    willDisplay cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    sections[indexPath.section].collectionView?(
      collectionView,
      willDisplay: cell,
      forItemAt: indexPath
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    didEndDisplaying cell: UICollectionViewCell,
    forItemAt indexPath: IndexPath
  ) {
    sections[indexPath.section].collectionView?(
      collectionView,
      didEndDisplaying: cell,
      forItemAt: indexPath
    )
  }
}

// MARK: - UICollectionViewDataSource
extension NewTabPageViewController: UICollectionViewDataSource {
  func numberOfSections(in collectionView: UICollectionView) -> Int {
    sections.count
  }
  func collectionView(
    _ collectionView: UICollectionView,
    numberOfItemsInSection section: Int
  ) -> Int {
    sections[section].collectionView(collectionView, numberOfItemsInSection: section)
  }
  func collectionView(
    _ collectionView: UICollectionView,
    cellForItemAt indexPath: IndexPath
  ) -> UICollectionViewCell {
    sections[indexPath.section].collectionView(collectionView, cellForItemAt: indexPath)
  }
  func collectionView(
    _ collectionView: UICollectionView,
    viewForSupplementaryElementOfKind kind: String,
    at indexPath: IndexPath
  ) -> UICollectionReusableView {
    sections[indexPath.section].collectionView?(
      collectionView,
      viewForSupplementaryElementOfKind: kind,
      at: indexPath
    ) ?? UICollectionReusableView()
  }
  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfigurationForItemsAt indexPaths: [IndexPath],
    point: CGPoint
  ) -> UIContextMenuConfiguration? {
    guard let indexPath = indexPaths.first else { return nil }
    return sections[indexPath.section].collectionView?(
      collectionView,
      contextMenuConfigurationForItemsAt: indexPaths,
      point: point
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    highlightPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    return sections[indexPath.section].collectionView?(
      collectionView,
      contextMenuConfiguration: configuration,
      highlightPreviewForItemAt: indexPath
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    contextMenuConfiguration configuration: UIContextMenuConfiguration,
    dismissalPreviewForItemAt indexPath: IndexPath
  ) -> UITargetedPreview? {
    return sections[indexPath.section].collectionView?(
      collectionView,
      contextMenuConfiguration: configuration,
      dismissalPreviewForItemAt: indexPath
    )
  }
  func collectionView(
    _ collectionView: UICollectionView,
    willPerformPreviewActionForMenuWith configuration: UIContextMenuConfiguration,
    animator: UIContextMenuInteractionCommitAnimating
  ) {
    guard let indexPath = configuration.identifier as? IndexPath else {
      return
    }
    sections[indexPath.section].collectionView?(
      collectionView,
      willPerformPreviewActionForMenuWith: configuration,
      animator: animator
    )
  }
}

// MARK: - UICollectionViewDragDelegate & UICollectionViewDropDelegate

extension NewTabPageViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {

  func collectionView(
    _ collectionView: UICollectionView,
    itemsForBeginning session: UIDragSession,
    at indexPath: IndexPath
  ) -> [UIDragItem] {
    // Check If the item that is dragged is a favourite item
    guard sections[indexPath.section] is FavoritesSectionProvider else {
      return []
    }

    let itemProvider = NSItemProvider(object: "\(indexPath)" as NSString)
    let dragItem = UIDragItem(itemProvider: itemProvider).then {
      $0.previewProvider = { () -> UIDragPreview? in
        guard let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell else {
          return nil
        }
        return UIDragPreview(view: cell.imageView)
      }
    }

    return [dragItem]
  }

  func collectionView(
    _ collectionView: UICollectionView,
    performDropWith coordinator: UICollectionViewDropCoordinator
  ) {
    guard let sourceIndexPath = coordinator.items.first?.sourceIndexPath else { return }
    let destinationIndexPath: IndexPath

    if let indexPath = coordinator.destinationIndexPath {
      destinationIndexPath = indexPath
    } else {
      let section = max(collectionView.numberOfSections - 1, 0)
      let row = collectionView.numberOfItems(inSection: section)
      destinationIndexPath = IndexPath(row: max(row - 1, 0), section: section)
    }

    guard sourceIndexPath.section == destinationIndexPath.section else { return }

    if coordinator.proposal.operation == .move {
      guard let item = coordinator.items.first else { return }
      _ = coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)

      guard let favouritesSection = sections.firstIndex(where: { $0 is FavoritesSectionProvider })
      else {
        return
      }

      Favorite.reorder(
        sourceIndexPath: sourceIndexPath,
        destinationIndexPath: destinationIndexPath,
        isInteractiveDragReorder: true
      )

      UIView.performWithoutAnimation {
        self.collectionView.reloadSections(IndexSet(integer: favouritesSection))
      }

    }
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropSessionDidUpdate session: UIDropSession,
    withDestinationIndexPath destinationIndexPath: IndexPath?
  ) -> UICollectionViewDropProposal {
    guard let destinationIndexSection = destinationIndexPath?.section,
      let favouriteSection = sections[destinationIndexSection] as? FavoritesSectionProvider,
      favouriteSection.hasMoreThanOneFavouriteItems
    else {
      return .init(operation: .cancel)
    }

    return .init(operation: .move, intent: .insertAtDestinationIndexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    fetchInteractionPreviewParameters(at: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dropPreviewParametersForItemAt indexPath: IndexPath
  ) -> UIDragPreviewParameters? {
    fetchInteractionPreviewParameters(at: indexPath)
  }

  func collectionView(
    _ collectionView: UICollectionView,
    dragSessionIsRestrictedToDraggingApplication session: UIDragSession
  ) -> Bool {
    return true
  }

  private func fetchInteractionPreviewParameters(at indexPath: IndexPath) -> UIDragPreviewParameters
  {
    let previewParameters = UIDragPreviewParameters().then {
      $0.backgroundColor = .clear

      if let cell = collectionView.cellForItem(at: indexPath) as? FavoritesCell {
        $0.visiblePath = UIBezierPath(roundedRect: cell.imageView.frame, cornerRadius: 8)
      }
    }

    return previewParameters
  }
}

extension NewTabPageViewController {
  private class NewTabCollectionView: UICollectionView {
    override init(frame: CGRect, collectionViewLayout layout: UICollectionViewLayout) {
      super.init(frame: frame, collectionViewLayout: layout)

      backgroundColor = .clear
      delaysContentTouches = false
      alwaysBounceVertical = true
      showsHorizontalScrollIndicator = false
      // Needed for some reason, as its not setting safe area insets while in landscape
      contentInsetAdjustmentBehavior = .always
      showsVerticalScrollIndicator = false
      // Even on light mode we use a darker background now
      indicatorStyle = .white

      // Drag should be enabled to rearrange favourite
      dragInteractionEnabled = true
    }
    @available(*, unavailable)
    required init(coder: NSCoder) {
      fatalError()
    }
    override func touchesShouldCancel(in view: UIView) -> Bool {
      return true
    }
  }
}

// MARK: - URL bar overlay
extension NewTabPageViewController {
  func searchContainerDidDismiss() {
    // Growser-290: no sponsored background impression to report.
  }
}

// Growser-281: no braveNewsFeatureUsage.

extension P3ATimedStorage where Value == Int {
  // Growser-281: no Brave News timed storages.
  fileprivate static var newTabsCreatedStorage: Self {
    .init(name: "new-tabs-created", lifetimeInDays: 7)
  }
  fileprivate static var sponsoredNewTabsCreatedStorage: Self {
    .init(name: "sponsored-new-tabs-created", lifetimeInDays: 7)
  }
}
