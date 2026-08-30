/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PackedIcon, packKeyFor } from './icon_pack'

// The server-side tail of the icon pack (growser#96). The bundled pack
// answers most boards without a request; a domain it does not know is
// asked of icon.growser.org ONCE, and the answer - either a tile drawn
// between browser releases, or "not there" - is cached here so the same
// domain is not asked again for days.
//
// The page NEVER touches the network itself (a WebUI page is a privileged
// context - lesson 53): the ask goes over mojo to the browser process,
// which sends the domain and nothing else, with no cookies.
export interface TailAnswer {
  status: number
  entryJson: string
}

export type TailFetcher = (domain: string) => Promise<TailAnswer>

async function askOverMojo(domain: string): Promise<TailAnswer> {
  const { NewTabPageProxy } = await import('../state/new_tab_page_proxy')
  return NewTabPageProxy.getInstance().handler.fetchPackTailIcon(domain)
}

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
  fetcher: TailFetcher = askOverMojo,
  factory: IDBFactory | undefined = globalThis.indexedDB,
  now: () => number = Date.now,
): Promise<PackedIcon | null> {
  const key = packKeyFor(url)
  if (!key || !factory) {
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
      const answer = await fetcher(key)
      if (answer.status === 200 && answer.entryJson) {
        const entry = JSON.parse(answer.entryJson)
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
      if (answer.status === 404) {
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
