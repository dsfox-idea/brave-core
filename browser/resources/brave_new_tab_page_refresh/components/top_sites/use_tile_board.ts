/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TopSite } from '../../state/top_sites_store'
import { CachedTile, readTileCache, writeTileCache } from '../../lib/tile_cache'
import { tileIconURL } from '../../lib/favicon_url'
import { tileColorFor } from '../../lib/tile_color'
import { tileLabel } from '../../lib/tile_label'
import { lookupPackedIcon } from '../../lib/icon_pack'
import { maxTileCount } from './tile_rows'

export type BoardTile = CachedTile

// The board a new tab paints, from three sources that arrive at three
// different times:
//
//   the cache        immediately, and it is what the last tab looked like
//   the browser      a moment later, and it is authoritative
//   the icon colours last, because each one costs decoding a favicon
//
// Only the first is on the path to the first frame, which is the whole point
// of keeping it.
export function useTileBoard(
  sites: TopSite[],
  initialized: boolean,
): BoardTile[] {
  const colorsRef = React.useRef(new Map<string, string>())
  const [cachedTiles, setCachedTiles] = React.useState<BoardTile[] | null>(null)
  const [cacheRead, setCacheRead] = React.useState(false)
  const [colorVersion, setColorVersion] = React.useState(0)

  React.useEffect(() => {
    let cancelled = false
    readTileCache()
      .then((tiles) => {
        if (cancelled || !tiles) {
          return
        }
        for (const tile of tiles) {
          // A tile cached without a colour is one whose favicon had not been
          // fetched yet. Seeding that would make it grey forever; leaving it out
          // means it is looked up again on this load.
          if (tile.color) {
            colorsRef.current.set(tile.url, tile.color)
          }
        }
        setCachedTiles(tiles)
      })
      .finally(() => {
        if (!cancelled) {
          setCacheRead(true)
        }
      })
    return () => {
      cancelled = true
    }
  }, [])

  const tiles = React.useMemo(() => {
    if (sites.length === 0) {
      // Before the browser answers, last time's board. Once it has answered -
      // which is what `initialized` says - an empty board is the truth, and
      // showing the cache instead would leave tiles up for sites the user has
      // just removed.
      return initialized ? [] : (cachedTiles ?? [])
    }
    return sites.slice(0, maxTileCount).map((site) => {
      // The pack first: a drawing we made and a colour we chose beat anything
      // read off a favicon, and asking it costs no request at all.
      const packed = lookupPackedIcon(site.url)
      if (packed) {
        return {
          url: site.url,
          title: site.title,
          label: tileLabel(site.title, site.url),
          color: packed.colour,
          kind: packed.kind,
          drawing: packed.drawing,
        }
      }
      return {
        url: site.url,
        title: site.title,
        label: tileLabel(site.title, site.url),
        color: colorsRef.current.get(site.url) ?? null,
      }
    })
    // `colorVersion` is what makes a resolved colour reach the board; the
    // colours themselves live in a ref so that resolving one does not rebuild
    // the map for all of them.
  }, [sites, cachedTiles, colorVersion, initialized])


  React.useEffect(() => {
    // Not before the cache has been read, or every colour it holds would be
    // looked up again while it was still on its way - which is the one cost
    // the cache exists to remove.
    if (!cacheRead) {
      return
    }
    let cancelled = false
    // Only sites the pack does not know: for the rest the colour is already
    // decided and reading a favicon would be work for nothing.
    const missing = sites
      .slice(0, maxTileCount)
      .filter((site) => !colorsRef.current.has(site.url)
        && !lookupPackedIcon(site.url))
    if (missing.length === 0) {
      return
    }
    Promise.all(
      missing.map(async (site) => {
        const color = await tileColorFor(tileIconURL(site.url))
        if (color) {
          colorsRef.current.set(site.url, color)
        }
      }),
    ).then(() => {
      if (!cancelled) {
        setColorVersion((version) => version + 1)
      }
    })
    return () => {
      cancelled = true
    }
  }, [sites, cacheRead])

  React.useEffect(() => {
    // Only ever cache a board the browser has confirmed, so that a cache read
    // that lost a race cannot write itself back as the truth. An empty board
    // is worth caching too, once confirmed - it clears a board the user has
    // emptied.
    if (initialized) {
      writeTileCache(tiles)
    }
  }, [tiles, initialized])

  return tiles
}
