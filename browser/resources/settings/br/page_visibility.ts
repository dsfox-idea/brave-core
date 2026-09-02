/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {loadTimeData} from 'chrome://resources/js/load_time_data.js'

import {
  pageVisibility as chromiumPageVisibility,
  resetPageVisibilityForTesting
} from '../page_visibility.js'

import type { PageVisibility } from '../page_visibility.js'

// Merge our interface additions with upstream's interface
declare module '../page_visibility.js' {
  export interface PageVisibility {
    braveSync?: boolean
    braveWallet?: boolean
    // <if expr="enable_containers">
    containers?: boolean
    // </if>
    content?: boolean
    getStarted?: boolean
    leoAssistant?: boolean
    leoPersonalization?: boolean
    leoModels?: boolean
    newTab?: boolean
    origin?: boolean
    playlist?: boolean
    shields?: boolean
    socialBlocking?: boolean
    // <if expr="enable_speedreader">
    speedreader?: boolean
    // </if>
    surveyPanelist?: boolean,
    braveTor?: boolean,
    emailAliases?: boolean,
    // <if expr="enable_psst">
    psst?: boolean,
    // </if>
  }
}

const alwaysTrue = {
  get: () => true
}

const alwaysTrueProxy = new Proxy({}, alwaysTrue)

function getPageVisibility () {
  // Use Chromium value defined in page_visibility.ts in guest mode
  // which hides most sections, and add Brave sections to hide.
  if (loadTimeData.getBoolean('isGuest')) {
    // Hide appropriate brave sections as well as chromium ones
    return {
      ...chromiumPageVisibility,
      braveSync: false,
      braveWallet: false,
      // <if expr="enable_containers">
      containers: false,
      // </if>
      content: false,
      getStarted: false,
      leoAssistant: false,
      leoPersonalization: false,
      leoModels: false,
      newTab: false,
      origin: false,
      playlist: false,
      shields: true,
      socialBlocking: true,
      // <if expr="enable_speedreader">
      speedreader: false,
      // </if>
      surveyPanelist: false,
      braveTor: false,
      emailAliases: false,
      // <if expr="enable_psst">
      psst: false,
      // </if>
    }
  }
  // We need to specify values for every attribute in pageVisibility instead of
  // only overriding specific attributes here because Chromium does not
  // explicitly define pageVisibility in page_visibility.ts since Polymer only
  // notifies after a property is set.
  // Use proxy objects here so we only need to write out the attributes we
  // would like to hide.
  // See brave/browser/resources/settings/br/basic_page.ts for Brave's list,
  // and chrome/browser/resources/settings/page_visibility.ts for Chromium's list.
  const staticProps = {
    // future-proof chromium actually defining something,
    ...chromiumPageVisibility,
    // overrides
    ai: false,
    people: false,
    defaultBrowser: false,
    onStartup: false,
    appearance: alwaysTrueProxy,
    privacy: alwaysTrueProxy,
    // custom properties
    braveSync: !loadTimeData.getBoolean('isSyncDisabled'),
    // growser: the Web3/Wallet and Leo sections are hidden from settings
    // (#16), the same way guest mode hides its own above. Hidden at the
    // page-visibility level (menu and routes); the C++ gates are left alone,
    // so their tests stay green. Removing them from the build is #4/#6.
    // <if expr="enable_brave_wallet">
    braveWallet: false,
    // </if>
    // <if expr="enable_ai_chat">
    leoAssistant: false,
    leoPersonalization: false,
    leoModels: false,
    // </if>
    // growser (#78): Brave's own research panel, and it reported through P3A,
    // which this build removed entirely (#21). Nothing behind the switch.
    surveyPanelist: false,
    // <if expr="enable_containers">
    containers: loadTimeData.getBoolean('isContainersEnabled'),
    // </if>
    content: alwaysTrueProxy,
    playlist: loadTimeData.getBoolean('isPlaylistFeatureEnabled') &&
              !loadTimeData.getBoolean('isPlaylistDisabledByPolicy'),
    // <if expr="enable_speedreader">
    speedreader: loadTimeData.getBoolean('isSpeedreaderAllowed'),
    // </if>
    // Growser-157: the Tor section is back. growser#78 hid it because the
    // client could not reach a fork; we publish our own component now, and the
    // pref this reads is the one thing that decides whether Tor is offered.
    // <if expr="enable_tor">
    braveTor: !loadTimeData.getBoolean('braveTorDisabledByPolicy') ||
              loadTimeData.getBoolean('shouldExposeElementsForTesting'),
    // </if>
    // growser (#78): the whole feature is an API against
    // aliases.bravesoftware.com - a Brave account service we have no account
    // with and no intention of running. There is nothing to configure.
    // <if expr="enable_email_aliases">
    emailAliases: false,
    // </if>
    // <if expr="enable_psst">
    psst: loadTimeData.getBoolean('isPsstEnabled'),
    // </if>
    origin: loadTimeData.getBoolean('isBraveOriginPurchased') &&
            !loadTimeData.getBoolean('isBraveOriginBrandedBuild'),
  }
  // Proxy so we can respond to any other property
  return new Proxy(staticProps, {
    get: function(target, prop) {
      if (prop in target) {
        return target[prop as keyof Object]
      }
      // default to allow, like chromium
      return true
    }
  })
}

// Provide an export in case our overrides want to explicitly import this
// override. Even though we are modifying chromium's override, the es module
// eval timing may result in the unoverridden value being obtained.
export const pageVisibility = getPageVisibility()
resetPageVisibilityForTesting(pageVisibility as PageVisibility)
