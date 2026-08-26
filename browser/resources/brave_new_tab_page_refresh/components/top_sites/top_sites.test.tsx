/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, render, waitFor } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'

import { TopSitesContext } from '../../context/top_sites_context'
import {
  TopSite,
  TopSitesListKind,
  TopSitesState,
  defaultTopSitesStore,
} from '../../state/top_sites_store'
import { TopSites } from './top_sites'

// jsdom implements neither the popover API nor <dialog>, and the menus and the
// edit modal call into both on mount. Nothing here is under test - without the
// stubs the component cannot render at all.
beforeAll(() => {
  const element = HTMLElement.prototype as any
  element.showPopover ??= () => {}
  element.hidePopover ??= () => {}
  const dialog = HTMLDialogElement.prototype as any
  dialog.showModal ??= () => {}
  dialog.close ??= () => {}
})

jest.mock('../../lib/tile_cache', () => ({
  readTileCache: () => Promise.resolve(null),
  writeTileCache: () => Promise.resolve(),
}))

jest.mock('../../lib/tile_color', () => ({
  tileColorFor: () => Promise.resolve('#33c57b'),
}))

function sites(count: number): TopSite[] {
  return Array.from(
    { length: count },
    (_, i) =>
      ({
        url: 'https://site' + i + '.example/',
        title: 'Site ' + i,
        favicon: '',
      }) as TopSite,
  )
}

function renderTopSites(state: Partial<TopSitesState>, actions = {}) {
  const store = createStateStore<TopSitesState>({
    ...defaultTopSitesStore().getState(),
    ...state,
    actions: { ...defaultTopSitesStore().getState().actions, ...actions },
  })
  return render(
    <TopSitesContext.Provider value={store}>
      <TopSites />
    </TopSitesContext.Provider>,
  )
}

// The board reads its cache and its colours asynchronously, so a test that
// asserts and returns leaves those updates landing on an unmounted tree. This
// lets them land first.
async function settle() {
  // Two turns: the cache read resolves on the first, and the colour lookup it
  // unblocks resolves on the next.
  await act(async () => {
    await Promise.resolve()
  })
  await act(async () => {
    await Promise.resolve()
  })
}

describe('TopSites', () => {
  it('renders nothing when the user has hidden the board', async () => {
    const { container } = renderTopSites({
      showTopSites: false,
      topSites: sites(4),
    })
    expect(container.querySelector('.top-site-tile')).toBeNull()
    await settle()
  })

  it('renders nothing when there are no sites', async () => {
    const { container } = renderTopSites({ showTopSites: true, topSites: [] })
    expect(container.querySelector('.top-site-tile')).toBeNull()
    await settle()
  })

  it('renders a tile per site', async () => {
    const { container } = renderTopSites({
      showTopSites: true,
      topSites: sites(3),
    })
    await waitFor(() =>
      expect(container.querySelectorAll('.top-site-tile')).toHaveLength(
        // Three sites and the add button.
        4,
      ),
    )
    await settle()
  })

  it('offers no add button once the board is full', async () => {
    const { container } = renderTopSites({
      showTopSites: true,
      topSites: sites(16),
    })
    await waitFor(() =>
      expect(container.querySelectorAll('.top-site-tile')).toHaveLength(16),
    )
    expect(container.querySelector('.add-tile')).toBeNull()
    await settle()
  })

  it('makes the list the user own before adding to it', async () => {
    const setTopSitesListKind = jest.fn()
    const { container } = renderTopSites(
      {
        showTopSites: true,
        topSites: sites(2),
        topSitesListKind: TopSitesListKind.kMostVisited,
      },
      { setTopSitesListKind },
    )
    const addButton = await waitFor(() => {
      const button = container.querySelector('.add-tile')
      expect(button).not.toBeNull()
      return button as HTMLElement
    })
    addButton.click()
    expect(setTopSitesListKind).toHaveBeenCalledWith(TopSitesListKind.kCustom)
    await settle()
  })

  it('leaves a list the user already owns alone', async () => {
    const setTopSitesListKind = jest.fn()
    const { container } = renderTopSites(
      {
        showTopSites: true,
        topSites: sites(2),
        topSitesListKind: TopSitesListKind.kCustom,
      },
      { setTopSitesListKind },
    )
    const addButton = await waitFor(() => {
      const button = container.querySelector('.add-tile')
      expect(button).not.toBeNull()
      return button as HTMLElement
    })
    addButton.click()
    expect(setTopSitesListKind).not.toHaveBeenCalled()
    await settle()
  })
})
