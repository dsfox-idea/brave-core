/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { useTopSitesActions } from '../../context/top_sites_context'
import { getString } from '../../lib/strings'
import { TopSitesTile } from './top_site_tile'
import { splitIntoRows } from './tile_rows'
import { BoardTile } from './use_tile_board'

type GridItem = { type: 'tile'; tile: BoardTile } | { type: 'add-button' }

interface Props {
  tiles: BoardTile[]
  canAddSite: boolean
  onAddTopSite: () => void
  onTopSiteContextMenu: (tile: BoardTile, event: React.MouseEvent) => void
}

export function TopSitesGrid(props: Props) {
  const actions = useTopSitesActions()

  // The add button is a tile like any other, so it takes a place in the
  // layout rather than hanging off the end of the last row.
  const rows = React.useMemo(() => {
    const items: GridItem[] = props.tiles.map((tile) => ({
      type: 'tile' as const,
      tile,
    }))
    if (props.canAddSite) {
      items.push({ type: 'add-button' })
    }
    return splitIntoRows(items)
  }, [props.tiles, props.canAddSite])

  return (
    <div className='top-site-rows'>
      {rows.map((row, rowIndex) => (
        <div
          key={rowIndex}
          className='top-site-row'
        >
          {row.map((item) =>
            item.type === 'add-button' ? (
              <button
                key='add-button'
                className='top-site-tile add-tile'
                onClick={props.onAddTopSite}
                title={getString(S.NEW_TAB_ADD_TOP_SITE_LABEL)}
              >
                <Icon name='plus-add' />
                <span className='top-site-label'>
                  {getString(S.NEW_TAB_ADD_TOP_SITE_LABEL)}
                </span>
              </button>
            ) : (
              <TopSitesTile
                key={item.tile.url}
                tile={item.tile}
                onContextMenu={(event) =>
                  props.onTopSiteContextMenu(item.tile, event)
                }
                onNavigate={actions.recordTopSiteClick}
              />
            ),
          )}
        </div>
      ))}
    </div>
  )
}
