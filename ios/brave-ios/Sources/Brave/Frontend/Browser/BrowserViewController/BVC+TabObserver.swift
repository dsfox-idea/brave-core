// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveUI
// Growser-287: no BraveWallet.
import Foundation
import Preferences
import Shared
import UIKit
import Web

extension BrowserViewController: TabObserver {
  public func tabDidCreateWebView(_ tab: some TabState) {
    tab.view.frame = webViewContainer.frame

    if tab.isVisible, let scrollView = tab.webViewProxy?.scrollView {
      toolbarVisibilityViewModel.beginObservingScrollView(scrollView)
    }

    if !FeatureList.kUseProfileWebViewConfiguration.enabled {
      installContentScriptHandlers(in: tab)
    }
  }

  public func tabWillDeleteWebView(_ tab: some TabState) {
    tab.browserData?.cancelQueuedAlerts()
    if let scrollView = tab.webViewProxy?.scrollView {
      toolbarVisibilityViewModel.endScrollViewObservation(scrollView)
    }
    tab.view.removeFromSuperview()
  }

  public func tabDidStartNavigation(_ tab: some TabState) {
    tab.contentBlocker?.clearPageStats()

    let visibleURL = tab.visibleURL

    if tab === tabManager.selectedTab {
      toolbarVisibilityViewModel.toolbarState = .expanded
      clearPageZoomDialog()

      // If we are going to navigate to a new page, refresh the translate status.
      updateTranslateURLBar(tab: tab, state: .unavailable)

      // If we are going to navigate to a new page, hide the reader mode button. Unless we
      // are going to a about:reader page. Then we keep it on screen: it will change status
      // (orange color) as soon as the page has loaded.
      if let url = visibleURL {
        if !url.isInternalURL(for: .readermode) {
          topToolbar.updateReaderModeState(.unavailable)
          hideReaderModeBar(animated: false)
        }
      }
    }

    // Growser-287: no wallet icon, panel or notification to reset on a new origin.
  }

  public func tabWasShown(_ tab: some TabState) {
    if #available(iOS 26.0, *) {
      updateWebViewObscuredInsets()
    }
  }

  public func tabDidCommitNavigation(_ tab: some TabState) {
    // Odd Chromium behaviour resets the web views obscured insets when a navigation starts due to
    // a bug with their fullscreen support, so we must set this again after a commit
    if #available(iOS 26.0, *) {
      updateWebViewObscuredInsets()
    }

    // Clear the current request url and the redirect source url
    // We don't need these values after the request has been comitted
    tab.currentRequestURL = nil
    tab.redirectSourceURL = nil
    tab.isInternalRedirect = false

    // Dismiss any alerts that are showing on page navigation.
    if let alert = tab.shownPromptAlert {
      alert.dismiss(animated: false)
    }

    // The toolbar and url bar changes can not be
    // on different tab than selected. Or the webview
    // previews and etc will effect the status
    guard tabManager.selectedTab === tab else {
      return
    }

    // Growser-287: no wallet panel or notification to dismiss.

    updateUIForReaderHomeStateForTab(tab)
    updateBackForwardActionStatus(for: tab)
  }

  public func tabDidCommitSameDocumentNavigation(_ tab: some TabState) {

    if !Preferences.Privacy.privateBrowsingOnly.value,
      !tab.isPrivate || Preferences.Privacy.persistentPrivateBrowsing.value
    {
      tabManager.preserveScreenshot(for: tab)
      tabManager.saveTab(tab)
    }
  }

  public func tabDidFinishNavigation(_ tab: some TabState) {
    if !Preferences.Privacy.privateBrowsingOnly.value
      && (!tab.isPrivate || Preferences.Privacy.persistentPrivateBrowsing.value)
    {
      tabManager.preserveScreenshot(for: tab)
      tabManager.saveTab(tab)
    }

    // Growser-283: no SKUS receipt to inject - SKUS is out of the product.

    navigateInTab(tab: tab)
    // Growser-290: no Rewards page-load reporting - Rewards is out.

    if let lastCommittedURL = tab.lastCommittedURL {
      maybeRecordBraveSearchDailyUsage(url: lastCommittedURL)
    }

    // Added this method to determine long press menu actions better
    // Since these actions are depending on tabmanager opened WebsiteCount
    updateToolbarUsingTabManager(tabManager)

    recordFinishedPageLoadP3A()
  }

  public func tab(_ tab: some TabState, didFailNavigationWithError error: any Error) {
    let error = error as NSError
    if error.code == Int(CFNetworkErrors.cfurlErrorCancelled.rawValue) {
      if tab === tabManager.selectedTab {
        if let displayURL = tab.visibleURL?.displayURL {
          updateToolbarCurrentURL(displayURL)
        } else if let url = tab.lastCommittedURL, !url.isLocal, !InternalURL.isValid(url: url) {
          updateToolbarCurrentURL(url.displayURL)
        }
        updateWebViewPageZoom(tab: tab)
      }
      return
    }
  }

  public func tabRenderProcessDidTerminate(_ tab: some TabState) {
    guard let url = tab.lastCommittedURL else { return }
    if url.isWebPage(includeDataURIs: false) {
      // For now just reload the page when the process crashes
      tab.reload()
    }
  }

  public func tabDidUpdateURL(_ tab: some TabState) {
    if tab.isDisplayingBasicAuthPrompt == true {
      tab.setVirtualURL(
        URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
      )
    }

    if tab === tabManager.selectedTab && !tab.isRestoring {
      updateUIForReaderHomeStateForTab(tab)
    }

    if tab.visibleURL?.origin == tab.previousCommittedURL?.origin {
      // Catch history pushState navigation, but ONLY for same origin navigation,
      // for reasons above about URL spoofing risk.
      navigateInTab(tab: tab)
    } else {
      updateURLBar()

      // If navigation will start from NTP, tab display url will be nil until
      // didCommit is called and it will cause url bar be empty in that period
      // To fix this when tab display url is empty, webview url is used
      if tab === tabManager.selectedTab, tab.visibleURL?.displayURL == nil {
        if let url = tab.visibleURL, !url.isLocal, !InternalURL.isValid(url: url) {
          updateToolbarCurrentURL(url.displayURL)
        }
      } else if tab === tabManager.selectedTab, tab.visibleURL?.displayURL?.scheme == "about",
        !tab.isLoading
      {
        if !tab.isRestoring {
          updateUIForReaderHomeStateForTab(tab)
        }

        navigateInTab(tab: tab)
      } else if tab === tabManager.selectedTab, let tabData = tab.browserData,
        tabData.isDisplayingBasicAuthPrompt
      {
        updateToolbarCurrentURL(
          URL(string: "\(InternalURL.baseUrl)/\(InternalURL.Path.basicAuth.rawValue)")
        )
      }
    }

    // Growser-290: no Rewards same-document reporting.

    // Update the estimated progress when the URL changes. Estimated progress may update to 0.1 when the url
    // is still an internal URL even though a request may be pending for a web page.
    if tab === tabManager.selectedTab, let url = tab.visibleURL,
      !url.isNewTabURL, !InternalURL.isValid(url: url), tab.isLoading, tab.estimatedProgress > 0
    {
      topToolbar.updateProgressBar(Float(tab.estimatedProgress))
    }

    Task {
      if self.tabManager.selectedTab === tab {
        self.updateToolbarSecureContentState(tab.visibleSecureContentState)
      }
    }
  }

  public func tabDidChangeLoadProgress(_ tab: some TabState) {
    guard tab === tabManager.selectedTab else { return }
    if let url = tab.visibleURL, !url.isNewTabURL, !InternalURL.isValid(url: url), tab.isLoading {
      topToolbar.updateProgressBar(Float(tab.estimatedProgress))
    } else {
      topToolbar.hideProgressBar()
    }
  }

  public func tabDidStartLoading(_ tab: some TabState) {
    guard tab === tabManager.selectedTab else { return }
    topToolbar.locationView.loading = tab.isLoading
  }

  public func tabDidStopLoading(_ tab: some TabState) {
    guard tab === tabManager.selectedTab else { return }
    topToolbar.locationView.loading = tab.isLoading
    if tab.estimatedProgress != 1 {
      topToolbar.updateProgressBar(1)
    }
  }

  public func tabDidChangeTitle(_ tab: some TabState) {
    // Ensure that the tab title *actually* changed to prevent repeated calls
    // to navigateInTab(tab:).
    guard
      let title = (tab.title?.isEmpty == true ? tab.visibleURL?.absoluteString : tab.title)
    else { return }
    if !title.isEmpty && title != tab.lastTitle {
      navigateInTab(tab: tab)
      tabsBar.updateSelectedTabTitle()
    }
  }

  public func tabDidChangeBackForwardState(_ tab: some TabState) {
    if tab !== tabManager.selectedTab { return }
    updateBackForwardActionStatus(for: tab)
  }

  public func tabDidChangeVisibleSecurityState(_ tab: some TabState) {
    if tabManager.selectedTab === tab {
      self.updateToolbarSecureContentState(tab.visibleSecureContentState)
    }
  }

  public func tabDidChangeSampledPageTopColor(_ tab: some TabState) {
    if tabManager.selectedTab === tab {
      updateStatusBarOverlayColor()
    }
  }
}

extension BrowserViewController {
  fileprivate func installContentScriptHandlers(in tab: some TabState) {
    var injectedScripts: [TabContentScript] = [
      BlockedDomainScriptHandler(),
      HTTPBlockedScriptHandler(tabManager: tabManager),
      PrintScriptHandler(browserController: self),
      DarkReaderScriptHandler(),
      BraveGetUA(),
      BraveSearchScriptHandler(profile: profile),  // Growser-290: no rewards
      ResourceDownloadScriptHandler(),
      AdsMediaReportingScriptHandler(),
      DeAmpScriptHandler(),
      SiteStateListenerScriptHandler(),
      CosmeticFiltersScriptHandler(),
      URLPartinessScriptHandler(),
      FaviconScriptHandler(),
      YoutubeQualityScriptHandler(),
      // Growser-279: no BraveLeoScriptHandler.
      RequestBlockingContentScriptHandler(),
    ]

    if let contentBlocker = tab.contentBlocker {
      injectedScripts.append(contentBlocker)
    }

    // Growser-282: no PlaylistScriptHandler.

    // Growser-278: no BraveTalkScriptHandler.

    // Growser-287: no Web3NameServiceScriptHandler.

    // Only add the logins handler, wallet provider and skus if the tab is NOT a private tab
    if !tab.isPrivate {
      injectedScripts += [
        LoginsScriptHandler(passwordAPI: profileController.passwordAPI),
        // Growser-290: no BraveSearchResultAdScriptHandler.
        // Growser-283: no BraveSkusScriptHandler.
      ]
      // Growser-287: no Ethereum, Solana or Cardano provider for pages.
    }

    if FeatureList.kBraveTranslateEnabled.enabled {
      injectedScripts.append(contentsOf: [
        BraveTranslateScriptLanguageDetectionHandler(),
        BraveTranslateScriptHandler(),
      ])
    }

    // XXX: Bug 1390200 - Disable NSUserActivity/CoreSpotlight temporarily
    // let spotlightHelper = SpotlightHelper(tab: tab)
    // tab.addHelper(spotlightHelper, name: SpotlightHelper.name())

    injectedScripts.forEach {
      tab.browserData?.addContentScript(
        $0,
        name: type(of: $0).scriptName,
        contentWorld: type(of: $0).scriptSandbox
      )
    }
  }
}
