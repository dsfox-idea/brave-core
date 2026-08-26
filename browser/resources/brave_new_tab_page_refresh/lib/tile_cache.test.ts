/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CachedTile, readTileCache, writeTileCache } from './tile_cache'

// jsdom ships no IndexedDB, so the test brings the smallest fake that still
// exercises what the module actually depends on: the upgrade callback running
// before success, the store having to exist before a transaction opens it, and
// every result arriving through an event rather than a return value.
function fakeIndexedDB() {
  const stores = new Map<string, Map<string, unknown>>()
  let created = false

  function fire<T>(target: any, result: T) {
    queueMicrotask(() => {
      target.result = result
      target.onsuccess?.()
    })
    return target
  }

  function objectStore(name: string) {
    const data = stores.get(name)
    if (!data) {
      throw new Error('no such object store: ' + name)
    }
    return {
      get: (key: string) => fire({} as any, data.get(key)),
      put: (value: unknown, key: string) => {
        data.set(key, value)
        return fire({} as any, key)
      },
    }
  }

  const database = {
    objectStoreNames: { contains: (name: string) => stores.has(name) },
    createObjectStore: (name: string) => stores.set(name, new Map()),
    transaction: () => ({ objectStore }),
    close: () => {},
  }

  return {
    factory: {
      open: () => {
        const open: any = {}
        queueMicrotask(() => {
          if (!created) {
            created = true
            open.result = database
            open.onupgradeneeded?.()
          }
          open.result = database
          open.onsuccess?.()
        })
        return open
      },
    } as unknown as IDBFactory,
    stores,
  }
}

const tiles: CachedTile[] = [
  { url: 'https://a.example/', title: 'A', label: 'A', color: '#33c57b' },
  { url: 'https://b.example/', title: 'B', label: 'B', color: null },
]

describe('tile_cache', () => {
  it('reads back what it wrote', async () => {
    const { factory } = fakeIndexedDB()
    await writeTileCache(tiles, factory)
    expect(await readTileCache(factory)).toEqual(tiles)
  })

  it('creates the store on first open', async () => {
    const { factory, stores } = fakeIndexedDB()
    expect(await readTileCache(factory)).toBeNull()
    expect(stores.has('top-sites')).toBe(true)
  })

  it('reports an empty cache rather than throwing without storage', async () => {
    expect(await readTileCache(undefined)).toBeNull()
    await expect(writeTileCache(tiles, undefined)).resolves.toBeUndefined()
  })

  it('rejects a record written in a shape it does not recognise', async () => {
    const { factory, stores } = fakeIndexedDB()
    await writeTileCache(tiles, factory)
    stores.get('top-sites')!.set('board', [{ url: 'https://a/', title: 'A' }])
    expect(await readTileCache(factory)).toBeNull()
  })

  it('rejects a record that is not a list', async () => {
    const { factory, stores } = fakeIndexedDB()
    await writeTileCache(tiles, factory)
    stores.get('top-sites')!.set('board', { url: 'https://a/' })
    expect(await readTileCache(factory)).toBeNull()
  })

  it('survives a database that refuses to open', async () => {
    const factory = {
      open: () => {
        const open: any = {}
        queueMicrotask(() => open.onerror?.())
        return open
      },
    } as unknown as IDBFactory
    expect(await readTileCache(factory)).toBeNull()
    await expect(writeTileCache(tiles, factory)).resolves.toBeUndefined()
  })
})
