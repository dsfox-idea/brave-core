/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  IconKind,
  PackedBrand,
  packedBrands as bundledBrands,
  packedDomains as bundledDomains,
  packedDrawings as bundledDrawings,
} from '../state/icon_pack'

// growser#190: the pack in force, which starts as the one compiled into this
// bundle and can be replaced by a newer one the browser hands over.
//
// The bundled pack is the FLOOR, not a first attempt: a fresh install draws a
// full board before any component has arrived, and a machine that never
// reaches us keeps the pack it shipped with. Nothing here can leave the board
// with fewer tiles than the browser was built with.
let packedDomains: Record<string, string> = bundledDomains
let packedBrands: Record<string, PackedBrand> = bundledBrands
let packedDrawings: Record<string, string> = bundledDrawings

// The format the browser and the pack have to agree on. A pack written for a
// LATER format is ignored rather than half-read - the bundled one is a
// correct pack, and half of a newer one is not.
const PACK_FORMAT = 1

let packRevision = 0
const packListeners = new Set<() => void>()

// Replaces the pack in force. Returns false and changes nothing when the
// payload is not a pack this browser understands, which is the same outcome
// as no pack at all.
export function installPack(packJson: string): boolean {
  if (!packJson) {
    return false
  }
  let parsed: any
  try {
    parsed = JSON.parse(packJson)
  } catch {
    return false
  }
  if (!parsed || parsed.format !== PACK_FORMAT) {
    return false
  }
  const { domains, brands, drawings } = parsed
  // Every table, or none: a pack missing its drawings would draw a board of
  // blank tiles, which is worse than the one already on screen.
  if (!isRecord(domains) || !isRecord(brands) || !isRecord(drawings)) {
    return false
  }
  packedDomains = domains
  packedBrands = brands
  packedDrawings = drawings
  packRevision += 1
  packListeners.forEach((listener) => listener())
  return true
}

// What is in force now. The board reads it so a render that happened before
// the pack arrived is not mistaken for one that happened after.
export function packedRevision() {
  return packRevision
}

export function onPackChanged(listener: () => void) {
  packListeners.add(listener)
  return () => {
    packListeners.delete(listener)
  }
}

function isRecord(value: unknown) {
  return Boolean(value) && typeof value === 'object' && !Array.isArray(value)
}

// Asks the browser for the pack it has. Once per page: the pack changes on the
// component updater's schedule, not while a tab is open.
//
// The page does not fetch this itself - a WebUI page is a privileged context
// and does not reach the network in this browser (lesson 53) - and what comes
// back is the WHOLE pack, so the ask says nothing about which sites are on
// this board (growser#96).
let packRequested = false

export async function loadPackFromBrowser(): Promise<boolean> {
  if (packRequested) {
    return false
  }
  packRequested = true
  try {
    const { NewTabPageProxy } = await import('../state/new_tab_page_proxy')
    const { packJson } =
      await NewTabPageProxy.getInstance().handler.getIconPack()
    return installPack(packJson)
  } catch {
    // A browser that cannot answer is a browser with nothing newer, and the
    // bundled pack is already drawn.
    return false
  }
}

// What the pack knows about a site: how to draw it, what colour its tile
// wears, and what to write on the tile when there is no drawing.
export interface PackedIcon {
  kind: IconKind
  colour: string
  name: string
  drawing: string | null
}

// A tile's url is a whole url and the pack is keyed by domain, with the www
// that some sites use and others do not stripped off both.
export function packKeyFor(url: string) {
  try {
    const host = new URL(url).hostname.toLowerCase()
    return host.startsWith('www.') ? host.slice(4) : host
  } catch {
    return ''
  }
}

// The pack holds the sites people are most likely to keep on a board. It is
// bundled with the browser rather than fetched, so drawing a board asks
// nothing of any server and tells none of them what is on it (growser#96).
export function lookupPackedIcon(url: string): PackedIcon | null {
  const key = packKeyFor(url)
  if (!key) {
    return null
  }
  // Both the domain and its registrable parent are tried, so that a tile
  // pointing at a section of a site - news.bbc.co.uk, or a country shop on
  // one of amazon's - still finds the brand.
  const brand = packedDomains[key] ?? packedDomains[parentDomain(key)]
  if (!brand) {
    return null
  }
  const entry = packedBrands[brand]
  if (!entry) {
    return null
  }
  return {
    kind: entry.kind,
    colour: entry.colour,
    name: entry.name,
    drawing: packedDrawings[brand] ?? null,
  }
}

// One label off the front, which turns news.bbc.co.uk into bbc.co.uk. Not a
// public suffix list: it is only ever used as a second guess against a table
// of known domains, so a wrong guess finds nothing rather than the wrong
// brand.
function parentDomain(host: string) {
  const parts = host.split('.')
  return parts.length > 2 ? parts.slice(1).join('.') : host
}

// A drawing as something an <img> can take.
//
// The page enforces Trusted Types, so an SVG cannot be handed to innerHTML at
// all - the first attempt did and the exception took the whole board down
// with it. A data URL needs no policy and no exception.
//
// A single-colour mark carries `currentColor`, which an <img> cannot inherit
// from the page: it is a document of its own. Since such a mark is always
// drawn white on the tile, the colour is written into it here.
export function drawingSource(drawing: string, mono: boolean) {
  const svg = mono ? drawing.replace(/currentColor/g, '#ffffff') : drawing
  return 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(svg)
}

export function packedDomainCount() {
  return Object.keys(packedDomains).length
}
