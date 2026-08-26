/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The most tiles the board holds. Two rows, no paging: past this the add
// button disappears rather than a third row appearing.
export const maxTileCount = 16

// How many tiles sit in the first row for a given total.
//
// A plain wrap would put the ninth tile alone on a second row, which reads as
// a mistake. Instead the first row gives tiles up so the second row starts at
// three, and only once the second row has filled to six does the first row go
// back to eight:
//
//   1..8   -> 8 across, one row       13 -> 8 + 5
//   9      -> 6 + 3                   14 -> 8 + 6
//   10     -> 6 + 4                   15 -> 8 + 7
//   11     -> 6 + 5                   16 -> 8 + 8
//   12     -> 6 + 6
export function firstRowCount(count: number) {
  if (count <= 8) {
    return count
  }
  return count <= 12 ? 6 : 8
}

// Splits items into the one or two rows the board renders. Anything past
// `maxTileCount` is dropped: callers cap the list before this, and a longer
// list here means a bug upstream rather than a third row.
export function splitIntoRows<T>(items: T[]): T[][] {
  const tiles = items.slice(0, maxTileCount)
  if (tiles.length === 0) {
    return []
  }
  const first = firstRowCount(tiles.length)
  const rows = [tiles.slice(0, first)]
  if (tiles.length > first) {
    rows.push(tiles.slice(first))
  }
  return rows
}
