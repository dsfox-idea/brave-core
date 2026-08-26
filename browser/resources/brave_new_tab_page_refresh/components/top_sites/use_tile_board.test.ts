/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { renderHook, waitFor } from '@testing-library/react'

import { CachedTile } from '../../lib/tile_cache'
import { TopSite } from '../../state/top_sites_store'
import { useTileBoard } from './use_tile_board'

let cachedTiles: CachedTile[] | null = null
let written: CachedTile[][] = []
let colors = new Map<string, string | null>()

jest.mock('../../lib/tile_cache', () => ({
  readTileCache: () => Promise.resolve(cachedTiles),
  writeTileCache: (tiles: CachedTile[]) => {
    written.push(tiles)
    return Promise.resolve()
  },
}))

jest.mock('../../lib/tile_color', () => ({
  tileColorFor: (iconURL: string) =>
    Promise.resolve(colors.get(iconURL) ?? null),
}))

function site(host: string, title: string): TopSite {
  return { url: 'https://' + host + '/', title, favicon: '' } as TopSite
}

function iconOf(host: string) {
  return (
    'chrome://growser-tile-icon/?pageUrl='
    + encodeURIComponent('https://' + host + '/')
  )
}

beforeEach(() => {
  cachedTiles = null
  written = []
  colors = new Map()
})

describe('use_tile_board', () => {
  it('paints the cached board before the browser has answered', async () => {
    cachedTiles = [
      { url: 'https://a/', title: 'A', label: 'A', color: '#112233' },
    ]
    const { result } = renderHook(() => useTileBoard([], false))
    expect(result.current).toEqual([])
    await waitFor(() => expect(result.current).toEqual(cachedTiles))
  })

  it('never writes a board the browser has not confirmed', async () => {
    cachedTiles = [
      { url: 'https://a/', title: 'A', label: 'A', color: '#112233' },
    ]
    const { result } = renderHook(() => useTileBoard([], false))
    await waitFor(() => expect(result.current.length).toBe(1))
    expect(written).toEqual([])
  })

  it('replaces the cached board with what the browser sent', async () => {
    cachedTiles = [
      { url: 'https://old/', title: 'Old', label: 'Old', color: '#112233' },
    ]
    const { result } = renderHook(() =>
      useTileBoard([site('new', 'New')], true),
    )
    await waitFor(() =>
      expect(result.current.map((tile) => tile.url)).toEqual(['https://new/']),
    )
  })

  it('shortens the title into a label', async () => {
    const { result } = renderHook(() =>
      useTileBoard([site('mail.example', 'Inbox - Example Mail')], true),
    )
    await waitFor(() => expect(result.current[0].label).toBe('Inbox'))
  })

  it('fills in the colour once the favicon has been read', async () => {
    colors.set(iconOf('a.example'), '#33c57b')
    const { result } = renderHook(() =>
      useTileBoard([site('a.example', 'A')], true),
    )
    expect(result.current[0].color).toBeNull()
    await waitFor(() => expect(result.current[0].color).toBe('#33c57b'))
    expect(written.at(-1)).toEqual([
      {
        url: 'https://a.example/',
        title: 'A',
        label: 'A',
        color: '#33c57b',
      },
    ])
  })

  it('reuses a cached colour instead of reading the favicon again', async () => {
    cachedTiles = [
      {
        url: 'https://a.example/',
        title: 'A',
        label: 'A',
        color: '#abcdef',
      },
    ]
    // The favicon would give a different colour; the cache must win, because
    // reading it is the cost the cache exists to avoid.
    colors.set(iconOf('a.example'), '#000000')
    const { result } = renderHook(() =>
      useTileBoard([site('a.example', 'A')], true),
    )
    await waitFor(() => expect(result.current[0].color).toBe('#abcdef'))
  })

  it('drops the cached board once the browser confirms it is empty', async () => {
    cachedTiles = [
      { url: 'https://gone/', title: 'Gone', label: 'Gone', color: '#112233' },
    ]
    const { result } = renderHook(() => useTileBoard([], true))
    await waitFor(() => expect(written.length).toBe(1))
    expect(result.current).toEqual([])
    expect(written.at(-1)).toEqual([])
  })

  it('caps the board at the number two rows hold', async () => {
    const sites = Array.from({ length: 30 }, (_, i) =>
      site('s' + i + '.example', 'S' + i),
    )
    const { result } = renderHook(() => useTileBoard(sites, true))
    await waitFor(() => expect(result.current.length).toBe(16))
  })
})
