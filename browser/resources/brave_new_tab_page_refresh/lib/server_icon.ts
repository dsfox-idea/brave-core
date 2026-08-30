/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PackedIcon, packKeyFor } from './icon_pack'

// The server-side tail of the icon pack (growser#96). The bundled pack
// answers most boards without a request; a domain it does not know is
// asked of icon.growser.org ONCE, and the answer - either a tile drawn
// between browser releases, or "not there" - is cached here so the same
// domain is not asked again for days. The request carries the domain and
// nothing else: no cookies, no identifier, no path of the page.
const ENDPOINT = 'https://icon.growser.org/icon?domain='

// An icon changes about as often as a company rebrands; a miss becomes a
// hit only when we draw the tile, which happens on the scale of days.
const HIT_TTL_MS = 7 * 24 * 60 * 60 * 1000
const MISS_TTL_MS = 24 * 60 * 60 * 1000

// Its own database rather than a second store in the board cache's one:
// bumping that database's version to add a store would make two tabs
// running different builds block each other's upgrade forever.
const databaseName = 'growser-server-icons'
const storeName = 'icons'

interface StoredAnswer {
  icon: PackedIcon | null
  fetchedAt: number
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
    open.onblocked = () => reject(new Error('blocked'))
  })
}

async function readStored(
  factory: IDBFactory,
  key: string,
): Promise<StoredAnswer | null> {
  const database = await openDatabase(factory)
  try {
    const transaction = database.transaction(storeName, 'readonly')
    const stored = await request<unknown>(
      transaction.objectStore(storeName).get(key))
    return (stored as StoredAnswer) ?? null
  } finally {
    database.close()
  }
}

async function writeStored(
  factory: IDBFactory,
  key: string,
  answer: StoredAnswer,
) {
  const database = await openDatabase(factory)
  try {
    const transaction = database.transaction(storeName, 'readwrite')
    transaction.objectStore(storeName).put(answer, key)
    await new Promise<void>((resolve, reject) => {
      transaction.oncomplete = () => resolve()
      transaction.onerror = () => reject(transaction.error)
    })
  } finally {
    database.close()
  }
}

// One request per domain per page, however many tiles ask at once.
const inFlight = new Map<string, Promise<PackedIcon | null>>()

export async function lookupServerIcon(
  url: string,
  // Resolved lazily rather than as `= fetch`: a default parameter is
  // evaluated at call time, and naming an undefined global there throws
  // before the body can decide anything (jsdom has no fetch).
  fetcher?: typeof fetch,
  factory: IDBFactory | undefined = globalThis.indexedDB,
  now: () => number = Date.now,
): Promise<PackedIcon | null> {
  const doFetch = fetcher ?? globalThis.fetch
  const key = packKeyFor(url)
  if (!key || !factory || !doFetch) {
    return null
  }
  const running = inFlight.get(key)
  if (running) {
    return running
  }
  const work = (async () => {
    try {
      const stored = await readStored(factory, key).catch(() => null)
      if (stored) {
        const ttl = stored.icon ? HIT_TTL_MS : MISS_TTL_MS
        if (now() - stored.fetchedAt < ttl) {
          return stored.icon
        }
      }
      const response = await doFetch(ENDPOINT + encodeURIComponent(key), {
        credentials: 'omit',
      })
      if (response.status === 200) {
        const entry = await response.json()
        const icon: PackedIcon = {
          kind: entry.kind,
          colour: entry.colour,
          name: entry.name ?? key,
          drawing: entry.drawing ?? null,
        }
        await writeStored(factory, key,
                          { icon, fetchedAt: now() }).catch(() => {})
        return icon
      }
      if (response.status === 404) {
        await writeStored(factory, key,
                          { icon: null, fetchedAt: now() }).catch(() => {})
      }
      // Any other status: a hiccup, not an answer. Nothing is cached, so
      // the next board draw simply asks again.
      return null
    } catch {
      return null
    } finally {
      inFlight.delete(key)
    }
  })()
  inFlight.set(key, work)
  return work
}
