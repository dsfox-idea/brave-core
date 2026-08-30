/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { tileIconURL } from '../../lib/favicon_url'
import { drawingSource, packKeyFor } from '../../lib/icon_pack'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { BoardTile } from './use_tile_board'
import { defaultTileColor } from './top_sites.style'

function sanitizeTileURL(url: string) {
  try {
    return new URL(url).toString()
  } catch {
    return ''
  }
}

// What to draw when a site has given us no icon at all. A letter is what the
// browser itself falls back to, and it reads as a deliberate tile rather than
// as a picture that failed to load.
function monogram(label: string, url: string) {
  const source = label || url.replace(/^https?:[/][/](www[.])?/, '')
  return source.trim().charAt(0).toUpperCase()
}

interface Props {
  tile: BoardTile
  onContextMenu?: (event: React.MouseEvent) => void
  onNavigate: () => void
}

export function TopSitesTile(props: Props) {
  const { color, drawing, kind, label, title, url } = props.tile
  const [iconMissing, setIconMissing] = React.useState(false)
  // The icon's own resolution, which is what caps how large it may be drawn.
  const [iconNatural, setIconNatural] = React.useState(0)

  React.useEffect(() => {
    setIconMissing(false)
    setIconNatural(0)
  }, [url])

  function onContextMenu(event: React.MouseEvent) {
    if (props.onContextMenu) {
      event.preventDefault()
      props.onContextMenu(event)
    }
  }

  // Three ways a tile can carry its site, in the order they look best.
  //
  // A drawing from the pack is ours: made once, checked by eye, and the same
  // on every machine. A single-colour mark is painted white, so the tile's
  // own colour lights it and no favicon's white background can show through
  // as a sticker.
  //
  // A brand the pack knows but could not draw shows its name, which is a
  // deliberate tile rather than a failure.
  //
  // Everything else falls back to the site's own icon, which is the path the
  // browser had before the pack existed and still the only answer for the
  // long tail of the web.
  function renderArt() {
    if (drawing) {
      return (
        <img
          className='top-site-art'
          src={drawingSource(drawing, kind === 'mono')}
          alt=''
        />
      )
    }
    if (kind === 'name') {
      return <span className='top-site-name'>{label || title}</span>
    }
    if (iconMissing) {
      return <span className='top-site-monogram'>{monogram(label, url)}</span>
    }
    return (
      <img
        className='top-site-icon'
        src={tileIconURL(url)}
        alt=''
        onLoad={(event) => setIconNatural(event.currentTarget.naturalWidth)}
        onError={() => setIconMissing(true)}
      />
    )
  }

  return (
    <a
      className={'top-site-tile' + (kind === 'name' ? ' named' : '')}
      href={sanitizeTileURL(url)}
      draggable={false}
      onClick={props.onNavigate}
      onContextMenu={onContextMenu}
      style={inlineCSSVars({
        '--self-tile-color': color || defaultTileColor,
        ...(iconNatural ? { '--self-icon-natural': iconNatural } : {}),
      })}
    >
      {renderArt()}
      <span className='top-site-label'>{packKeyFor(url) || label}</span>
    </a>
  )
}
