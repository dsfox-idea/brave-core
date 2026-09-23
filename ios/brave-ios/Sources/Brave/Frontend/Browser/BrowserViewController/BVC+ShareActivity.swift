// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
// Growser-281: no BraveNews.
import Web

extension BrowserViewController {
  func presentActivityViewController(
    _ url: URL,
    tab: (any TabState)? = nil,
    source: SharePopoverSource
  ) {
    presentShareActivity(
      url: url,
      tab: tab,
      syncAPI: profileController.syncAPI,
      sendTabAPI: profileController.sendTabAPI,
      // Growser-281: no feedDataSource, and Brave News is never available.
      isBraveNewsAvailable: false,
      source: source,
      callbacks: .init(
        onToggleReaderMode: { [weak tab] in
          tab?.readerMode?.toggleReaderMode()
        },
        onDisplayPageZoom: { [weak self] in self?.displayPageZoomDialog() },
        onAddSearchEngine: { [weak self, weak tab] in
          guard let self, let tab else { return }
          self.evaluateWebsiteSupportOpenSearchEngine(in: tab)
          self.addCustomSearchEngineForFocusedElement()
        },
        onDisplayCertificate: { [weak self] in self?.displayPageCertificateInfo() },
        onShowSubmitReport: { [weak self] url in self?.showSubmitReportView(for: url) },
        onCleanUp: { [weak self] in self?.showQueuedAlertIfAvailable() }
      )
    )
  }
}
