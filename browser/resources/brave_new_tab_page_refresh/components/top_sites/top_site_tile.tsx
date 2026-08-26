/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { tileIconURL } from '../../lib/favicon_url'
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
  const { color, label, title, url } = props.tile
  const [iconMissing, setIconMissing] = React.useState(false)

  React.useEffect(() => {
    setIconMissing(false)
  }, [url])

  function onContextMenu(event: React.MouseEvent) {
    if (props.onContextMenu) {
      event.preventDefault()
      props.onContextMenu(event)
    }
  }

  return (
    <a
      className='top-site-tile'
      href={sanitizeTileURL(url)}
      title={title}
      draggable={false}
      onClick={props.onNavigate}
      onContextMenu={onContextMenu}
      style={inlineCSSVars({ '--self-tile-color': color || defaultTileColor })}
    >
      {iconMissing ? (
        <span className='top-site-monogram'>{monogram(label, url)}</span>
      ) : (
        <img
          className='top-site-icon'
          src={tileIconURL(url)}
          alt=''
          onError={() => setIconMissing(true)}
        />
      )}
      <span className='top-site-label'>{label}</span>
    </a>
  )
}
