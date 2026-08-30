/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { lookupServerIcon } from './server_icon'

// The same smallest-possible IndexedDB fake the tile cache test uses: the
// module only needs get/put through events.
function fakeIndexedDB() {
  const stores = new Map<string, Map<string, unknown>>()

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

  return {
    open: () => {
      const openRequest: any = {}
      queueMicrotask(() => {
        openRequest.result = {
          objectStoreNames: {
            contains: (name: string) => stores.has(name),
          },
          createObjectStore: (name: string) => {
            stores.set(name, new Map())
          },
          transaction: (name: string) => {
            const transaction: any = { objectStore: () => objectStore(name) }
            queueMicrotask(() => {
              queueMicrotask(() => transaction.oncomplete?.())
            })
            return transaction
          },
          close: () => {},
        }
        openRequest.onupgradeneeded?.()
        openRequest.onsuccess?.()
      })
      return openRequest
    },
  } as unknown as IDBFactory
}

function answering(status: number, body?: unknown) {
  const calls: string[] = []
  const fetcher = (async (input: RequestInfo | URL) => {
    calls.push(String(input))
    return {
      status,
      json: async () => body,
    } as Response
  }) as typeof fetch
  return { fetcher, calls }
}

const TILE = {
  kind: 'full',
  colour: '#33c57b',
  name: 'Drawn Brand',
  drawing: '<svg xmlns="http://www.w3.org/2000/svg"/>',
}

describe('server_icon', () => {
  it('asks with the domain and nothing else, and returns the tile', async () => {
    const { fetcher, calls } = answering(200, TILE)
    const icon = await lookupServerIcon(
      'https://www.drawn-brand.example/page?q=1', fetcher, fakeIndexedDB())
    expect(calls).toEqual(
      ['https://icon.growser.org/icon?domain=drawn-brand.example'])
    expect(icon?.colour).toBe('#33c57b')
    expect(icon?.drawing).toContain('<svg')
  })

  it('caches an answer so the same domain is not asked twice', async () => {
    const { fetcher, calls } = answering(200, TILE)
    const factory = fakeIndexedDB()
    await lookupServerIcon('https://a.example/', fetcher, factory)
    const again = await lookupServerIcon('https://a.example/', fetcher, factory)
    expect(calls.length).toBe(1)
    expect(again?.name).toBe('Drawn Brand')
  })

  it('caches a miss, and asks again only after its day expires', async () => {
    const { fetcher, calls } = answering(404)
    const factory = fakeIndexedDB()
    let clock = 1_000_000
    const now = () => clock
    expect(await lookupServerIcon('https://b.example/', fetcher, factory,
                                  now)).toBeNull()
    expect(await lookupServerIcon('https://b.example/', fetcher, factory,
                                  now)).toBeNull()
    expect(calls.length).toBe(1)
    clock += 25 * 60 * 60 * 1000
    await lookupServerIcon('https://b.example/', fetcher, factory, now)
    expect(calls.length).toBe(2)
  })

  it('treats a server hiccup as no answer and caches nothing', async () => {
    const { fetcher, calls } = answering(503)
    const factory = fakeIndexedDB()
    expect(await lookupServerIcon('https://c.example/', fetcher,
                                  factory)).toBeNull()
    await lookupServerIcon('https://c.example/', fetcher, factory)
    expect(calls.length).toBe(2)
  })

  it('answers null without a network when the url is not one', async () => {
    const { fetcher, calls } = answering(200, TILE)
    expect(await lookupServerIcon('not a url', fetcher,
                                  fakeIndexedDB())).toBeNull()
    expect(calls.length).toBe(0)
  })
})
