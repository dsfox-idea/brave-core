/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { fireEvent, render } from '@testing-library/react'

import { TopSitesGrid } from './top_sites_grid'
import { BoardTile } from './use_tile_board'

function tiles(count: number): BoardTile[] {
  return Array.from({ length: count }, (_, i) => ({
    url: 'https://site' + i + '.example/',
    title: 'Site ' + i,
    label: 'Site ' + i,
    color: '#33c57b',
  }))
}

function renderGrid(count: number, canAddSite = false, onAdd = () => {}) {
  const result = render(
    <TopSitesGrid
      tiles={tiles(count)}
      canAddSite={canAddSite}
      onAddTopSite={onAdd}
      onTopSiteContextMenu={() => {}}
    />,
  )
  const rows = Array.from(result.container.querySelectorAll('.top-site-row'))
  return {
    ...result,
    rowLengths: rows.map(
      (row) => row.querySelectorAll('.top-site-tile').length,
    ),
  }
}

describe('top_sites_grid', () => {
  it('lays a small board out in one row', () => {
    expect(renderGrid(5).rowLengths).toEqual([5])
    expect(renderGrid(8).rowLengths).toEqual([8])
  })

  it('splits a larger board the way the layout rule says', () => {
    expect(renderGrid(9).rowLengths).toEqual([6, 3])
    expect(renderGrid(12).rowLengths).toEqual([6, 6])
    expect(renderGrid(13).rowLengths).toEqual([8, 5])
    expect(renderGrid(16).rowLengths).toEqual([8, 8])
  })

  it('counts the add button as a tile in the layout', () => {
    // Eight sites plus the add button is nine tiles, so the board splits.
    expect(renderGrid(8, true).rowLengths).toEqual([6, 3])
  })

  it('paints each tile in the colour it was given', () => {
    const { container } = renderGrid(3)
    const tile = container.querySelector('.top-site-tile') as HTMLElement
    expect(tile.style.getPropertyValue('--self-tile-color')).toBe('#33c57b')
  })

  it('does not offer a tile for dragging', () => {
    const { container } = renderGrid(2)
    const tile = container.querySelector('.top-site-tile') as HTMLElement
    expect(tile.getAttribute('draggable')).toBe('false')
  })

  it('captions every tile with its address, not its title', () => {
    const { container } = renderGrid(3)
    const labels = Array.from(
      container.querySelectorAll('.top-site-label'),
    ).map((node) => node.textContent)
    expect(labels).toEqual([
      'site0.example',
      'site1.example',
      'site2.example',
    ])
  })

  it('offers no tooltip on a tile', () => {
    const { container } = renderGrid(2)
    const tile = container.querySelector('.top-site-tile') as HTMLElement
    expect(tile.getAttribute('title')).toBeNull()
  })

  it('calls back when the add button is pressed', () => {
    const onAdd = jest.fn()
    const { container } = renderGrid(2, true, onAdd)
    fireEvent.click(container.querySelector('.add-tile') as HTMLElement)
    expect(onAdd).toHaveBeenCalledTimes(1)
  })

  it('renders no add button when the caller says there is no room', () => {
    expect(renderGrid(16).container.querySelector('.add-tile')).toBeNull()
  })

  it('renders nothing at all for an empty board', () => {
    expect(renderGrid(0).rowLengths).toEqual([])
  })
})
