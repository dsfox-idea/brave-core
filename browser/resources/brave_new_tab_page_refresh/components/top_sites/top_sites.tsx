/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'

import { TopSite, TopSitesListKind } from '../../state/top_sites_store'
import {
  useTopSitesState,
  useTopSitesActions,
} from '../../context/top_sites_context'
import { getString } from '../../lib/strings'
import { RemoveToast } from './remove_toast'
import { TopSitesGrid } from './top_sites_grid'
import { TopSiteEditModal } from './top_site_edit_modal'
import { BoardTile, useTileBoard } from './use_tile_board'
import { maxTileCount } from './tile_rows'
import { useDevicePixelRatio } from '../../lib/device_pixel_ratio'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { Popover } from '../common/popover'

import { style } from './top_sites.style'

export function TopSites() {
  const actions = useTopSitesActions()

  const showTopSites = useTopSitesState((s) => s.showTopSites)
  const listKind = useTopSitesState((s) => s.topSitesListKind)
  const topSites = useTopSitesState((s) => s.topSites)
  const initialized = useTopSitesState((s) => s.initialized)

  const [showEditSite, setShowEditSite] = React.useState(false)
  const [editSite, setEditSite] = React.useState<TopSite | null>(null)
  const [showTopSitesMenu, setShowTopSitesMenu] = React.useState(false)
  const [contextMenuTile, setContextMenuTile] =
    React.useState<BoardTile | null>(null)
  const [showRemoveToast, setShowRemoveToast] = React.useState(false)

  const rootRef = React.useRef<HTMLDivElement>(null)
  const tiles = useTileBoard(topSites, initialized)
  const devicePixelRatio = useDevicePixelRatio()

  const canAddSite = tiles.length < maxTileCount

  function setContextMenuPosition(event: React.MouseEvent) {
    const elem = rootRef.current
    if (elem) {
      elem.style.setProperty('--self-context-menu-x', event.pageX + 'px')
      elem.style.setProperty('--self-context-menu-y', event.pageY + 'px')
    }
  }

  function onTopSiteContextMenu(tile: BoardTile, event: React.MouseEvent) {
    setContextMenuTile(tile)
    setContextMenuPosition(event)
  }

  function topSitesMenuAction(fn: () => void) {
    return () => {
      fn()
      setShowTopSitesMenu(false)
    }
  }

  function onAddTopSite() {
    // Adding a site is only meaningful for a list the user owns. Switching
    // carries the current tiles over as they are (MostVisitedSites seeds the
    // custom list from them), so the board does not change under the user -
    // it just becomes theirs to edit.
    if (listKind === TopSitesListKind.kMostVisited) {
      actions.setTopSitesListKind(TopSitesListKind.kCustom)
    }
    setEditSite(null)
    setShowEditSite(true)
  }

  if (!showTopSites || tiles.length === 0) {
    return null
  }

  return (
    <div
      ref={rootRef}
      className='allow-background-pointer-events'
      data-css-scope={style.scope}
      style={inlineCSSVars({ '--self-dpr': devicePixelRatio })}
    >
      <div className='top-site-context-menu-anchor' />
      <div className='top-sites'>
        <div className='left-spacer allow-background-pointer-events' />
        <TopSitesGrid
          tiles={tiles}
          canAddSite={canAddSite}
          onAddTopSite={onAddTopSite}
          onTopSiteContextMenu={onTopSiteContextMenu}
        />
        <button
          className='menu-button'
          onClick={() => setShowTopSitesMenu(true)}
        >
          <Icon name='more-vertical' />
        </button>
        <Popover
          isOpen={showTopSitesMenu}
          className='top-sites-menu'
          onClose={() => setShowTopSitesMenu(false)}
        >
          <div className='popover-menu'>
            {canAddSite && (
              <button onClick={topSitesMenuAction(onAddTopSite)}>
                <Icon name='browser-add' />
                {getString(S.NEW_TAB_ADD_TOP_SITE_LABEL)}
              </button>
            )}
            <div className='menu-divider' />
            {listKind === TopSitesListKind.kCustom ? (
              <button
                onClick={topSitesMenuAction(() =>
                  actions.setTopSitesListKind(TopSitesListKind.kMostVisited),
                )}
              >
                <Icon name='history' />
                {getString(S.NEW_TAB_TOP_SITES_SHOW_MOST_VISITED_LABEL)}
              </button>
            ) : (
              <button
                onClick={topSitesMenuAction(() =>
                  actions.setTopSitesListKind(TopSitesListKind.kCustom),
                )}
              >
                <Icon name='star-outline' />
                {getString(S.NEW_TAB_TOP_SITES_SHOW_CUSTOM_LABEL)}
              </button>
            )}
            <div className='menu-divider' />
            <button
              onClick={topSitesMenuAction(() => actions.setShowTopSites(false))}
            >
              <Icon name='eye-off' />
              {getString(S.NEW_TAB_HIDE_TOP_SITES_LABEL)}
            </button>
          </div>
        </Popover>
        <Popover
          isOpen={Boolean(contextMenuTile)}
          className='top-site-context-menu'
          onClose={() => setContextMenuTile(null)}
        >
          <div className='popover-menu'>
            {listKind === TopSitesListKind.kCustom && (
              <button
                onClick={() => {
                  const url = contextMenuTile?.url
                  setEditSite(topSites.find((site) => site.url === url) ?? null)
                  setShowEditSite(true)
                  setContextMenuTile(null)
                }}
              >
                <Icon name='edit-pencil' />
                {getString(S.NEW_TAB_EDIT_TOP_SITE_LABEL)}
              </button>
            )}
            <button
              onClick={() => {
                if (contextMenuTile) {
                  actions.removeTopSite(contextMenuTile.url)
                  setContextMenuTile(null)
                  setShowRemoveToast(true)
                }
              }}
            >
              <Icon name='trash' />
              {getString(S.NEW_TAB_REMOVE_TOP_SITE_LABEL)}
            </button>
          </div>
        </Popover>
        <TopSiteEditModal
          topSite={editSite}
          isOpen={showEditSite}
          onSave={(url, title) => {
            if (editSite) {
              actions.updateTopSite(editSite.url, url, title)
            } else {
              actions.addTopSite(url, title)
            }
            setShowEditSite(false)
          }}
          onClose={() => {
            setShowEditSite(false)
          }}
        />
        <RemoveToast
          isOpen={showRemoveToast}
          onUndo={() => {
            actions.undoRemoveTopSite()
            setShowRemoveToast(false)
          }}
          onClose={() => {
            setShowRemoveToast(false)
          }}
        />
      </div>
    </div>
  )
}
