// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import WebKit
import XCTest

@testable import Brave

// growser#347: the translate element script asks for main.js and its CSS; they
// must come from the host that served the element script.
final class BraveTranslateMirrorTests: XCTestCase {
  override class func setUp() {
    BraveCoreMain.initializeResourceBundleForTesting()
  }

  @MainActor func testScriptFromTheMirrorLoadsItsFilesFromTheMirror() async throws {
    let host = try await hostOfMainJS(elementScriptFrom: "ru.growser.org")
    XCTAssertEqual(host, "ru.growser.org")
  }

  @MainActor func testScriptFromTheBackendLoadsItsFilesFromTheBackend() async throws {
    let host = try await hostOfMainJS(elementScriptFrom: "backend.growser.org")
    XCTAssertEqual(host, "backend.growser.org")
  }

  // Runs the shipped translate user script with an element script that, like
  // the real one, asks for main.js, and returns the host that request went to.
  @MainActor private func hostOfMainJS(elementScriptFrom host: String) async throws -> String? {
    let elementScript = BraveTranslateScriptHandler.elementScript(
      "cr.googleTranslate.onLoadJavascript("
        + "'https://translate.googleapis.com/translate_static/js/element/main.js');",
      from: host
    )
    let viewController = MockScriptsViewController()
    let stream = viewController.attachScriptHandler(
      contentWorld: BraveTranslateScriptHandler.scriptSandbox,
      name: BraveTranslateScriptHandler.messageHandlerName,
      messageHandler: MockMessageHandler(timeout: 30) { message in
        let command = (message.body as? [String: Any])?["command"] as? String
        if command == "load_brave_translate_script" {
          return elementScript
        }
        return [
          "value": ["statusCode": 200, "responseType": "", "response": "", "headers": ""]
        ]
      }
    )
    viewController.loadViewIfNeeded()
    // translate.js (IDR_TRANSLATE_JS) lives in resources.pak, which
    // initializeResourceBundleForTesting does not load, so the user script has
    // no cr.googleTranslate, and its first catch calls the missing
    // onTranslateElementError, which stops the script. The script overrides
    // onLoadJavascript itself; it only needs the object and that handler.
    viewController.add(
      userScript: WKUserScript(
        source: "var cr = cr || {}; cr.googleTranslate = cr.googleTranslate || {};"
          + "cr.googleTranslate.onTranslateElementError = (e) => console.error(e);",
        injectionTime: .atDocumentStart,
        forMainFrameOnly: true,
        in: BraveTranslateScriptHandler.scriptSandbox
      )
    )
    viewController.add(userScript: try XCTUnwrap(BraveTranslateScriptHandler.userScript))
    try await viewController.loadHTMLStringAndWait("<html><body>Hello</body></html>")

    _ = try? await viewController.webView.callAsyncJavaScript(
      "await window.__firefox__.\(BraveTranslateScriptHandler.namespace).loadTranslateScript();",
      contentWorld: BraveTranslateScriptHandler.scriptSandbox
    )

    for try await message in stream {
      guard let body = message.body as? [String: Any], body["command"] as? String == "request"
      else { continue }
      return URL(string: body["url"] as? String ?? "")?.host
    }
    return nil
  }
}
