/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// A render-ready cache of the top sites board.
//
// Everything the board needs to paint - the sites, their labels and the colour
// taken from each favicon - is written here as one record, so a new tab paints
// the board it painted last time without waiting for the browser to answer and
// without re-reading a single favicon. The browser's answer arrives a moment
// later and corrects it.
//
// IndexedDB rather than local storage: local storage is synchronous and blocks
// the main thread on the very frame we are trying to make cheap.

const databaseName = 'growser-new-tab'
const storeName = 'top-sites'
const recordKey = 'board'

export interface CachedTile {
  url: string
  title: string
  label: string
  color: string | null
  // From the icon pack, when it knows this site: 'mono' and 'full' come with
  // a drawing, 'name' means the tile carries the site's name instead.
  kind?: 'mono' | 'full' | 'name'
  drawing?: string | null
}

function request<T>(source: IDBRequest<T>): Promise<T> {
  return new Promise((resolve, reject) => {
    source.onsuccess = () => resolve(source.result)
    source.onerror = () => reject(source.error)
  })
}

function openDatabase(factory: IDBFactory): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const open = factory.open(databaseName, 1)
    open.onupgradeneeded = () => {
      if (!open.result.objectStoreNames.contains(storeName)) {
        open.result.createObjectStore(storeName)
      }
    }
    open.onsuccess = () => resolve(open.result)
    open.onerror = () => reject(open.error)
    // A second tab holding an older version open would block the upgrade
    // forever; the board is a cache, so give up rather than hang.
    open.onblocked = () => reject(new Error('tile cache upgrade blocked'))
  })
}

// Anything unreadable is treated as an empty cache: this is a cache, and a
// browser that refuses storage (or a record written by an older build) must
// cost a slower first paint, never a broken page.
function parseTiles(value: unknown): CachedTile[] | null {
  if (!Array.isArray(value)) {
    return null
  }
  const tiles: CachedTile[] = []
  for (const entry of value) {
    if (!entry || typeof entry !== 'object') {
      return null
    }
    const { url, title, label, color, kind, drawing } =
      entry as Record<string, unknown>
    if (
      typeof url !== 'string'
      || typeof title !== 'string'
      || typeof label !== 'string'
      || (color !== null && typeof color !== 'string')
    ) {
      return null
    }
    // Written back exactly as it was read: a record from a build that knew
    // nothing of the pack must not come back with empty pack fields bolted
    // on, or every read would look like a change and rewrite the cache.
    const tile: CachedTile = { url, title, label, color }
    if (kind === 'mono' || kind === 'full' || kind === 'name') {
      tile.kind = kind
    }
    if (typeof drawing === 'string') {
      tile.drawing = drawing
    }
    tiles.push(tile)
  }
  return tiles
}

export async function readTileCache(
  factory: IDBFactory | undefined = globalThis.indexedDB,
): Promise<CachedTile[] | null> {
  if (!factory) {
    return null
  }
  let database: IDBDatabase | null = null
  try {
    database = await openDatabase(factory)
    const store = database
      .transaction(storeName, 'readonly')
      .objectStore(storeName)
    return parseTiles(await request(store.get(recordKey)))
  } catch {
    return null
  } finally {
    database?.close()
  }
}

export async function writeTileCache(
  tiles: CachedTile[],
  factory: IDBFactory | undefined = globalThis.indexedDB,
): Promise<void> {
  if (!factory) {
    return
  }
  let database: IDBDatabase | null = null
  try {
    database = await openDatabase(factory)
    const store = database
      .transaction(storeName, 'readwrite')
      .objectStore(storeName)
    await request(store.put(tiles, recordKey))
  } catch {
    // Losing the cache costs a slower first paint next time, nothing more.
  } finally {
    database?.close()
  }
}
