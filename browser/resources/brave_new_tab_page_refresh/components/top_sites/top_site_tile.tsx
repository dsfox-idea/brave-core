/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { faviconURL } from '../../lib/favicon_url'
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

interface Props {
  tile: BoardTile
  onContextMenu?: (event: React.MouseEvent) => void
  onNavigate: () => void
}

export function TopSitesTile(props: Props) {
  const { color, label, title, url } = props.tile

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
      <img
        className='top-site-icon'
        src={faviconURL(url)}
        alt=''
      />
      <span className='top-site-label'>{label}</span>
    </a>
  )
}
