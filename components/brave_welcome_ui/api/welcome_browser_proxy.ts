// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.js'
import { DefaultBrowserBrowserProxyImpl } from './default_browser_browser_proxy'
import {
  ImportDataBrowserProxyImpl,
  ImportDataStatus,
  BrowserProfile as _BrowserProfile,
} from './import_data_browser_proxy'

export interface BrowserProfile extends _BrowserProfile {
  browserType?: string | undefined
}

export const defaultImportTypes = {
  import_dialog_autofill_form_data: true,
  import_dialog_bookmarks: true,
  import_dialog_history: true,
  import_dialog_saved_passwords: true,
  import_dialog_search_engine: true,
  import_dialog_extensions: true,
  import_dialog_payments: true
}

export interface WelcomeBrowserProxy {
  openSettingsPage: () => void
  getDefaultBrowser: () => Promise<string>
  getWelcomeCompleteURL: () => Promise<string>
}

export {
  DefaultBrowserBrowserProxyImpl,
  ImportDataBrowserProxyImpl,
  ImportDataStatus,
}

export class WelcomeBrowserProxyImpl implements WelcomeBrowserProxy {
  openSettingsPage () {
    chrome.send('openSettingsPage')
  }

  getDefaultBrowser (): Promise<string> {
    return sendWithPromise('getDefaultBrowser')
  }

  getWelcomeCompleteURL (): Promise<string> {
    return sendWithPromise('getWelcomeCompleteURL')
  }

  static getInstance (): WelcomeBrowserProxy {
    return instance || (instance = new WelcomeBrowserProxyImpl())
  }

  static setInstance (obj: WelcomeBrowserProxy) {
    instance = obj
  }
}

let instance: WelcomeBrowserProxyImpl | null = null
