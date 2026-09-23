// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import BraveUI
// Growser-280: no BraveVPN.
import BraveWallet
import BrowserMenu
import Data
import Foundation
// Growser-282: no PlaylistUI.
import Preferences
import Shared
import SwiftUI
import Web
import os.log

extension BrowserViewController {
  private var settingsController: SettingsViewController {
    let isPrivateMode = privateBrowsingManager.isPrivateBrowsing
    let keyringService = BraveWallet.KeyringServiceFactory.get(privateMode: isPrivateMode)
    let walletService = BraveWallet.ServiceFactory.get(privateMode: isPrivateMode)
    let rpcService = BraveWallet.JsonRpcServiceFactory.get(privateMode: isPrivateMode)

    var keyringStore: KeyringStore? = walletStore?.keyringStore
    if keyringStore == nil {
      if let keyringService = keyringService,
        let walletService = walletService,
        let rpcService = rpcService
      {
        keyringStore = KeyringStore(
          keyringService: keyringService,
          walletService: walletService,
          rpcService: rpcService
        )
      }
    }

    var cryptoStore: CryptoStore? = walletStore?.cryptoStore
    if cryptoStore == nil {
      cryptoStore = CryptoStore.from(
        ipfsApi: profileController.ipfsAPI,
        privateMode: isPrivateMode
      )
    }

    let vc = SettingsViewController(
      profile: self.profile,
      tabManager: self.tabManager,
      // Growser-281: no feedDataSource.
      rewards: self.rewards,
      windowProtection: self.windowProtection,
      p3aUtils: self.braveCore.p3aUtils,
      braveCore: self.profileController,
      localState: self.braveCore.localState,
      attributionManager: attributionManager,
      keyringStore: keyringStore,
      cryptoStore: cryptoStore
    )
    vc.settingsDelegate = self
    return vc
  }

  /// Presents Wallet without an origin (ex. from menu)
  func presentWallet() {
    self.dismiss(animated: true) {
      self.tabManager.addTabAndSelect(
        URLRequest(url: .webUI.wallet.home),
        isPrivate: self.privateBrowsingManager.isPrivateBrowsing
      )
    }
  }

  /// Present Native Wallet from a Wallet WebUI Action
  func presentNativeWallet(webUIAction: WalletWebUIAction) {
    guard let walletStore = self.walletStore ?? newWalletStore() else { return }
    walletStore.origin = nil
    let presentingContext: PresentingContext = .webUI(action: webUIAction)
    let vc = WalletHostingViewController(
      walletStore: walletStore,
      webImageDownloader: profileController.webImageDownloader,
      presentingContext: presentingContext
    )
    vc.delegate = self
    self.dismiss(animated: true) {
      self.present(vc, animated: true)
    }
  }

  // Growser-282: no presentPlaylistController().

  func presentBrowserMenu(
    from sourceView: UIView,
    activities: [UIActivity],
    tab: (any TabState)?,
    pageURL: URL?
  ) {
    var actions: [Action] = []
    // Growser-280: no VPN menu action.
    actions.append(contentsOf: destinationMenuActions(for: pageURL))
    actions.append(contentsOf: pageActions(for: pageURL, tab: tab))
    var pageActivities: Set<Action> = Set(
      activities
        .compactMap { activity in
          guard let id = (activity as? MenuActivity)?.id,
            let actionID = Action.Identifier.allPageActivites.first(where: { $0.id == id })
          else {
            return nil
          }
          return (activity, actionID)
        }
        .map { (activity: UIActivity, actionID: Action.Identifier) in
          .init(id: actionID) { @MainActor [unowned self] _ in
            self.dismiss(animated: true) {
              activity.perform()
            }
            return .none
          }
        }
    )
    if let tab,
      let requestDesktopPageActivity = pageActivities.first(where: { $0.id == .requestDesktopSite })
    {
      // Remove the UIActivity version and replace it with a manual version.
      // The request desktop activity is special in the sense that it is dynamic based on the
      // current tab user agent, but we don't use rely on the UIActivity information to populate
      // actions in the new menu UI, so this replaces it with how we would compose it manually
      pageActivities.remove(requestDesktopPageActivity)
      pageActivities.insert(
        .init(
          id: .requestDesktopSite,
          title: tab.currentUserAgentType == .desktop
            ? Strings.appMenuViewMobileSiteTitleString : nil,
          image: tab.currentUserAgentType == .desktop ? "leo.smartphone" : nil,
          handler: { @MainActor [unowned self, weak tab] _ in
            tab?.switchUserAgent()
            self.dismiss(animated: true)
            return .none
          }
        )
      )
    }
    // Sets up empty actions for any page actions that weren't setup as UIActivity's excluding any
    // that should be hidden due to admin policies
    var pageActivitiesRemovedByAdminPolicies: Set<Action.Identifier> = []
    // Growser-281: Brave News is out of the product, so its "add source" entry
    // is always removed, not only by policy.
    pageActivitiesRemovedByAdminPolicies.insert(.addSourceNews)
    let remainingPageActivities: [Action] = Action.ID.allPageActivites
      .subtracting(pageActivities.map(\.id))
      .subtracting(pageActivitiesRemovedByAdminPolicies)
      .map { .init(id: $0, attributes: .disabled) }
    actions.append(contentsOf: pageActivities)
    actions.append(contentsOf: remainingPageActivities)
    let browserMenu = BrowserMenuController(
      actions: actions,
      handlePresentation: { [unowned self] action in
        switch action {
        case .settings:
          let vc = self.settingsController
          self.dismiss(animated: true) {
            self.presentSettingsNavigation(with: vc)
          }
        case .vpnRegionPicker:
          break  // Growser-280: unreachable - the menu never shows a connected VPN.
        }
      }
    )
    if UIDevice.current.userInterfaceIdiom == .pad {
      browserMenu.modalPresentationStyle = .popover
    }
    browserMenu.popoverPresentationController?.sourceView = sourceView
    browserMenu.popoverPresentationController?.sourceRect = sourceView.bounds
    browserMenu.popoverPresentationController?.popoverLayoutMargins = .init(equalInset: 4)
    browserMenu.popoverPresentationController?.permittedArrowDirections = [.up, .down]
    present(browserMenu, animated: true)
    return
  }

  private func pageActions(for pageURL: URL?, tab: (any TabState)?) -> [Action] {
    var actions: [Action] = [
      .init(id: .share) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.tabToolbarDidPressShare()
        }
        return .none
      },
      .init(id: .addBookmark) { @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          self.openAddBookmark()
        }
        return .none
      },
      .init(
        id: .toggleNightMode,
        state: Preferences.General.nightModeEnabled.value
      ) { @MainActor action in
        var actionCopy = action
        Preferences.General.nightModeEnabled.value.toggle()
        actionCopy.state = Preferences.General.nightModeEnabled.value
        return .updateAction(actionCopy)
      },
    ]
    // Growser-282: no "add to playlist" page action.
    if BraveCore.FeatureList.kBraveShredFeature.enabled {
      let isShredAvailable = tabManager.selectedTab?.visibleURL?.isShredAvailable ?? false
      actions.append(
        .init(id: .shredData, attributes: isShredAvailable ? [] : [.disabled]) {
          @MainActor [unowned self] _ in
          self.dismiss(animated: true) {
            guard let tab = self.tabManager.selectedTab, let url = tab.visibleURL else { return }
            let alert = UIAlertController.shredDataAlert(url: url) { _ in
              self.shredData(for: url, in: tab)
            }
            self.present(alert, animated: true)
          }
          return .none
        }
      )
    }
    let printFormatter = tab?.view.viewPrintFormatter()
    actions.append(
      .init(id: .print) {
        @MainActor [unowned self] _ in
        self.dismiss(animated: true) {
          let printController = UIPrintInteractionController.shared
          printController.printFormatter = printFormatter
          printController.present(animated: true)
        }
        return .none
      }
    )
    if pageURL == nil {
      for index in actions.indices {
        actions[index].attributes.insert(.disabled)
      }
    }
    return actions
  }

  // Growser-280: no vpnMenuAction - the VPN is out of the product.

  private func destinationMenuActions(for pageURL: URL?) -> [Action] {
    let isPrivateBrowsing = privateBrowsingManager.isPrivateBrowsing
    var actions: [Action] = [
      .init(id: .bookmarks) { @MainActor [unowned self] _ in
        let vc = BookmarksViewController(
          folder: bookmarkManager.lastVisitedFolder(),
          bookmarkManager: bookmarkManager,
          isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
        )
        vc.toolbarUrlActionsDelegate = self
        let container = UINavigationController(rootViewController: vc)
        self.dismiss(animated: true) {
          self.present(container, animated: true)
        }
        return .none
      },
      .init(id: .history) { @MainActor [unowned self] _ in
        let vc = UIHostingController(
          rootView: HistoryView(
            model: HistoryModel(
              api: self.profileController.historyAPI,
              tabManager: self.tabManager,
              toolbarUrlActionsDelegate: self,
              dismiss: { [weak self] in self?.dismiss(animated: true) },
              askForAuthentication: self.askForLocalAuthentication,
              serpMetrics: SerpMetricsServiceFactory.get(
                profile: self.profileController.profile
              )
            )
          )
        )
        self.dismiss(animated: true) {
          self.present(vc, animated: true)
        }
        return .none
      },
      .init(id: .downloads) { @MainActor [unowned self] _ in
        UIApplication.shared.openBraveDownloadsFolder { success in
          if !success {
            self.dismiss(animated: true) {
              self.displayOpenDownloadsError()
            }
          }
        }
        return .none
      },
    ]
    // Growser-282: no Playlist menu item.
    if profileController.braveWalletAPI.isAllowed {
      actions.append(
        .init(
          id: .braveWallet,
          attributes: isPrivateBrowsing ? .disabled : []
        ) { @MainActor [unowned self] _ in
          // Present wallet already handles dismiss + present
          self.presentWallet()
          return .none
        }
      )
    }
    // Growser-279: no Leo menu item.
    // Growser-278: no Brave Talk menu item.
    // Growser-281: no Brave News menu item.
    return actions
  }

}
