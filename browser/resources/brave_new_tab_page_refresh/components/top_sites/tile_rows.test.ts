/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { firstRowCount, maxTileCount, splitIntoRows } from './tile_rows'

function rowLengths(count: number) {
  const items = Array.from({ length: count }, (_, i) => i)
  return splitIntoRows(items).map((row) => row.length)
}

describe('tile_rows', () => {
  it('keeps a single row up to eight tiles', () => {
    for (let count = 1; count <= 8; ++count) {
      expect(rowLengths(count)).toEqual([count])
    }
  })

  it('pulls the first row down to six rather than stranding one tile', () => {
    expect(rowLengths(9)).toEqual([6, 3])
    expect(rowLengths(10)).toEqual([6, 4])
    expect(rowLengths(11)).toEqual([6, 5])
    expect(rowLengths(12)).toEqual([6, 6])
  })

  it('goes back to eight in the first row once the second row is full', () => {
    expect(rowLengths(13)).toEqual([8, 5])
    expect(rowLengths(14)).toEqual([8, 6])
    expect(rowLengths(15)).toEqual([8, 7])
    expect(rowLengths(16)).toEqual([8, 8])
  })

  it('never leaves the second row longer than the first', () => {
    for (let count = 9; count <= maxTileCount; ++count) {
      const [first, second] = rowLengths(count)
      expect(second).toBeLessThanOrEqual(first)
    }
  })

  it('never renders more than two rows or more than the maximum', () => {
    for (let count = 0; count <= maxTileCount + 8; ++count) {
      const rows = rowLengths(count)
      expect(rows.length).toBeLessThanOrEqual(2)
      expect(rows.reduce((sum, n) => sum + n, 0)).toBe(
        Math.min(count, maxTileCount),
      )
    }
  })

  it('has no gap between the rows it fills', () => {
    // Every tile lands in exactly one row, in order.
    const items = Array.from({ length: 13 }, (_, i) => i)
    expect(splitIntoRows(items).flat()).toEqual(items)
  })

  it('reports the first row count on its own', () => {
    expect(firstRowCount(0)).toBe(0)
    expect(firstRowCount(8)).toBe(8)
    expect(firstRowCount(9)).toBe(6)
    expect(firstRowCount(13)).toBe(8)
  })
})
