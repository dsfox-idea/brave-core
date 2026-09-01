/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { SearchBox } from './search/search_box'
import { Background } from './background/background'
import { TopSites } from './top_sites/top_sites'
import { Clock } from './common/clock'
import { useSearchLayoutReady } from './app_layout_ready'

import { style } from './app.style'

// <if expr="enable_ai_chat">
import { useNewTabState } from '../context/new_tab_context'
import { LazyQueryBox } from './query_box/lazy_query_box'
// </if>

// growser (#136): the page is three things - the board, the search row and the
// clock - and nothing else. Everything customizable went with the settings
// that reached it: the gear, the clock's own click, the search box's
// "customize" entry, the widget stacks, the news feed, the encryption promo,
// the photo caption and the ?openSettings= deep link that let the rest of the
// browser open this page's settings.
//
// What is gone is the WAY IN, not the feature. SettingsModal, the widgets, the
// news feed and the background service are all still compiled and still work;
// putting an element back here restores its screen. That keeps the diff small,
// keeps the conflict surface with upstream low on every Chromium bump, and
// makes the whole thing revertible.
//
// The one surface deliberately left room for is the informational widget the
// mac side is building (#118) - "important infobars stay" means that one.
export function App() {
  const searchLayoutReady = useSearchLayoutReady()

  return (
    <div data-css-scope={style.scope}>
      <Background />
      <div className='background-filter allow-background-pointer-events' />
      <main className='allow-background-pointer-events'>
        <div className='topsites-container allow-background-pointer-events'>
          <TopSites />
        </div>
        <div className='searchbox-container allow-background-pointer-events'>
          {searchLayoutReady && <Search />}
        </div>
        <div className='clock allow-background-pointer-events'>
          <Clock />
        </div>
      </main>
    </div>
  )
}

function Search() {
  // <if expr="enable_ai_chat">
  const aiChatInputEnabled = useNewTabState((s) => s.aiChatInputEnabled)
  if (aiChatInputEnabled) {
    // AI chat is compiled out of this build, so this branch never renders -
    // but webpack still type-checks it, and upstream's prop is required.
    return <LazyQueryBox showSearchSettings={() => {}} />
  }
  // </if>
  return <SearchBox />
}
