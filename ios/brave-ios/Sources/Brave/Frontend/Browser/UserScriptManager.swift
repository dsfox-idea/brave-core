// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
// Growser-287: no BraveWallet.
import Data
import Preferences
import Shared
import Web
import WebKit
import os.log

private class ScriptLoader: TabContentScriptLoader {}

class UserScriptManager {
  static let shared = UserScriptManager()

  static let securityToken = ScriptLoader.uniqueID
  static let walletSolanaNameSpace = "W\(ScriptLoader.uniqueID)"

  private var alwaysEnabledScripts: [ScriptType] {
    var scripts: [ScriptType] = [
      .faviconFetcher,
      .resourceDownloader,
      .nightMode,
    ]

    // Growser-282: never .playlist - Playlist is out of the product.

    if Preferences.UserScript.youtubeQuality.value {
      scripts.append(.youtubeQuality)
    }

    // Growser-279: never .braveLeoAIChat - Leo is out of the product.

    return scripts
  }

  /// Scripts that are loaded after `staticScripts`
  private let dynamicScripts: [ScriptType: WKUserScript] = {
    ScriptType.allCases.reduce(into: [:]) { $0[$1] = $1.script }
  }()

  /// Scripts that are web-packed and should be loaded after `baseScripts` but before `dynamicScripts`
  private let staticScripts: [WKUserScript] = {
    return [
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: true, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: true, sandboxed: true),
    ].compactMap { (injectionTime, mainFrameOnly, sandboxed) in

      let name =
        (mainFrameOnly ? "MainFrame" : "AllFrames") + "AtDocument"
        + (injectionTime == .atDocumentStart ? "Start" : "End") + (sandboxed ? "Sandboxed" : "")

      if let source = ScriptLoader.loadUserScript(named: name) {
        let wrappedSource =
          "(function() { const SECURITY_TOKEN = '\(UserScriptManager.securityToken)'; \(source) })()"

        return WKUserScript(
          source: wrappedSource,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page
        )
      }

      return nil
    }
  }()

  /// Scripts injected before all other scripts.
  private let baseScripts: [WKUserScript] = {
    [
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: false),
      (WKUserScriptInjectionTime.atDocumentStart, mainFrameOnly: false, sandboxed: true),
      (WKUserScriptInjectionTime.atDocumentEnd, mainFrameOnly: false, sandboxed: true),
    ].compactMap { (injectionTime, mainFrameOnly, sandboxed) in

      if let source = ScriptLoader.loadUserScript(named: "__firefox__") {
        return WKUserScript(
          source: source,
          injectionTime: injectionTime,
          forMainFrameOnly: mainFrameOnly,
          in: sandboxed ? .defaultClient : .page
        )
      }

      return nil
    }
  }()

  // Growser-287: no wallet provider scripts - the wallet is out.

  enum ScriptType: String, CaseIterable {
    case faviconFetcher
    case cookieBlocking
    case mediaBackgroundPlay
    case playlistMediaSource
    case playlist
    case nightMode
    case deAmp
    case requestBlocking
    case trackerProtectionStats
    case resourceDownloader
    case ethereumProvider
    case solanaProvider
    case cardanoProvider
    case searchResultAd
    case youtubeQuality
    case braveLeoAIChat
    case braveTranslate

    fileprivate var script: WKUserScript? {
      switch self {
      // Conditionally enabled scripts
      case .cookieBlocking:
        return Preferences.UserScript.cookieBlocking.value
          ? loadScript(named: "CookieControlScript") : nil
      case .mediaBackgroundPlay:
        return Preferences.UserScript.mediaBackgroundPlay.value
          ? loadScript(named: "MediaBackgroundingScript") : nil
      case .playlistMediaSource:
        return nil  // Growser-282: PlaylistSwizzlerScript is not bundled.
      case .deAmp: return Preferences.UserScript.deAmp.value ? DeAmpScriptHandler.userScript : nil
      case .requestBlocking:
        return Preferences.UserScript.requestBlocking.value
          ? RequestBlockingContentScriptHandler.userScript : nil
      case .trackerProtectionStats:
        return Preferences.UserScript.trackingProtectionStats.value
          ? ContentBlockerHelper.userScript : nil
      case .ethereumProvider, .solanaProvider, .cardanoProvider:
        return nil  // Growser-287: the provider script handlers are not built.
      case .searchResultAd: return BraveSearchResultAdScriptHandler.userScript

      // Always enabled scripts
      case .faviconFetcher: return FaviconScriptHandler.userScript
      case .nightMode: return DarkReaderScriptHandler.userScript
      case .playlist:
        return nil  // Growser-282: PlaylistScriptHandler is not built.
      case .resourceDownloader: return ResourceDownloadScriptHandler.userScript
      case .youtubeQuality:
        return Preferences.UserScript.youtubeQuality.value
          ? YoutubeQualityScriptHandler.userScript : nil
      case .braveLeoAIChat:
        return nil  // Growser-279: BraveLeoScriptHandler is not built.
      case .braveTranslate:
        return Preferences.UserScript.translate.value && FeatureList.kBraveTranslateEnabled.enabled
          ? BraveTranslateScriptHandler.userScript : nil
      }
    }

    private func loadScript(named: String) -> WKUserScript? {
      guard var script = ScriptLoader.loadUserScript(named: named) else {
        return nil
      }

      script = ScriptLoader.secureScript(handlerNamesMap: [:], securityToken: "", script: script)
      return WKUserScript(
        source: script,
        injectionTime: .atDocumentStart,
        forMainFrameOnly: false,
        in: .page
      )
    }
  }

  // Growser-287: no fetchWalletScripts(from:).

  public func loadScripts(
    into userContentController: WKUserContentController,
    scripts: Set<ScriptType>,
    tab: any TabState
  ) {
    if FeatureList.kUseProfileWebViewConfiguration.enabled
      || Preferences.UserScript.blockAllScripts.value
    {
      return
    }

    var scripts = scripts

    userContentController.do { scriptController in
      scriptController.removeAllUserScripts()
      tab.updateScripts()

      // Inject all base scripts
      self.baseScripts.forEach {
        scriptController.addUserScript($0)
      }

      // Inject specifically trackerProtectionStats BEFORE request blocking
      // this is because it needs to hook requests before requestBlocking
      if scripts.contains(.trackerProtectionStats),
        let script = self.dynamicScripts[.trackerProtectionStats]
      {
        scripts.remove(.trackerProtectionStats)
        scriptController.addUserScript(script)
      }

      // Inject specifically RequestBlocking BEFORE other scripts
      // this is because it needs to hook requests before RewardsReporting
      if scripts.contains(.requestBlocking), let script = self.dynamicScripts[.requestBlocking] {
        scripts.remove(.requestBlocking)
        scriptController.addUserScript(script)
      }

      // Inject all static scripts
      self.staticScripts.forEach {
        scriptController.addUserScript($0)
      }

      // Inject all scripts that are dynamic, but always enabled
      self.dynamicScripts.filter({ self.alwaysEnabledScripts.contains($0.key) }).forEach {
        scriptController.addUserScript($0.value)
      }

      // Inject all optional scripts
      self.dynamicScripts.filter({ scripts.contains($0.key) }).forEach {
        scriptController.addUserScript($0.value)
      }
    }
  }

  // TODO: Get rid of this OR refactor wallet and domain scripts
  func loadCustomScripts(
    into tab: some TabState,
    userScripts: Set<ScriptType>,
    customScripts: Set<UserScriptType>
  ) {
    if FeatureList.kUseProfileWebViewConfiguration.enabled
      || Preferences.UserScript.blockAllScripts.value
    {
      return
    }

    guard let userContentController = tab.configuration?.userContentController else {
      return
    }

    let logComponents = [
      userScripts.sorted(by: { $0.rawValue < $1.rawValue }).map { scriptType in
        " \(scriptType.rawValue)"
      }.joined(separator: "\n"),
      customScripts.sorted(by: { $0.order < $1.order }).map { scriptType in
        " #\(scriptType.order) \(scriptType.debugDescription)"
      }.joined(separator: "\n"),
    ]
    ContentBlockerManager.log.debug(
      "Loaded \(userScripts.count + customScripts.count) script(s): \n\(logComponents.joined(separator: "\n"))"
    )

    loadScripts(into: userContentController, scripts: userScripts, tab: tab)

    userContentController.do { scriptController in
      // Growser-287: no Ethereum, Solana or Cardano provider for pages.

      // TODO: Refactor this and get rid of the `UserScriptType`
      // Inject Custom scripts
      for userScriptType in customScripts.sorted(by: { $0.order < $1.order }) {
        do {
          let script = try ScriptFactory.shared.makeScript(for: userScriptType)
          scriptController.addUserScript(script)
        } catch {
          assertionFailure(
            "Should never happen. The scripts are packed in the project and loading/modifying should always be possible."
          )
          Logger.module.error("\(error.localizedDescription)")
        }
      }
    }
  }
}
