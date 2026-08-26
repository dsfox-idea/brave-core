/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  IconKind,
  packedBrands,
  packedDomains,
  packedDrawings,
} from '../state/icon_pack'

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
