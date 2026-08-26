/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { dominantColorOf, hslToHex, rgbToHSL } from './tile_color'

// Builds an RGBA buffer from a list of [r, g, b, a, repeat] runs.
function pixels(...runs: [number, number, number, number, number][]) {
  const out: number[] = []
  for (const [r, g, b, a, repeat] of runs) {
    for (let i = 0; i < repeat; ++i) {
      out.push(r, g, b, a)
    }
  }
  return new Uint8ClampedArray(out)
}

function lightnessOf(hex: string) {
  const value = parseInt(hex.slice(1), 16)
  return rgbToHSL(value >> 16, (value >> 8) & 0xff, value & 0xff).l
}

describe('tile_color', () => {
  it('returns null for a fully transparent icon', () => {
    expect(dominantColorOf(pixels([255, 0, 0, 0, 64]))).toBeNull()
  })

  it('returns null for an empty buffer', () => {
    expect(dominantColorOf(new Uint8ClampedArray())).toBeNull()
  })

  it('takes the coloured mark over the white field around it', () => {
    // A white background outnumbers the mark ten to one, which is what a real
    // favicon looks like. Weighting by saturation is what keeps it losing.
    const color = dominantColorOf(
      pixels([255, 255, 255, 255, 400], [220, 30, 40, 255, 40]),
    )
    const { h } = rgbToHSL(
      parseInt(color!.slice(1, 3), 16),
      parseInt(color!.slice(3, 5), 16),
      parseInt(color!.slice(5, 7), 16),
    )
    expect(h).toBeGreaterThan(340)
    expect(h % 360).toBeLessThan(370)
  })

  it('ignores transparent pixels entirely', () => {
    // The green is transparent, so the red must win despite being outnumbered.
    const color = dominantColorOf(
      pixels([0, 255, 0, 0, 500], [200, 20, 20, 255, 10]),
    )
    expect(color).not.toBeNull()
    const value = parseInt(color!.slice(1), 16)
    expect(value >> 16).toBeGreaterThan((value >> 8) & 0xff)
  })

  it('darkens a white icon into the readable range', () => {
    const color = dominantColorOf(pixels([255, 255, 255, 255, 64]))
    expect(lightnessOf(color!)).toBeCloseTo(0.67, 2)
  })

  it('lightens a black icon into the readable range', () => {
    const color = dominantColorOf(pixels([0, 0, 0, 255, 64]))
    expect(lightnessOf(color!)).toBeCloseTo(0.15, 2)
  })

  it('leaves a mid-tone colour where it is', () => {
    const color = dominantColorOf(pixels([51, 197, 123, 255, 64]))
    expect(color).toBe('#33c57b')
  })

  it('adds up shades of one colour instead of splitting the vote', () => {
    // Four near-identical blues, against one red that beats any of them alone.
    const color = dominantColorOf(
      pixels(
        [20, 40, 200, 255, 6],
        [22, 42, 202, 255, 6],
        [24, 44, 204, 255, 6],
        [26, 46, 206, 255, 6],
        [200, 30, 30, 255, 20],
      ),
    )
    const value = parseInt(color!.slice(1), 16)
    expect(value & 0xff).toBeGreaterThan(value >> 16)
  })

  it('round-trips a colour through HSL', () => {
    for (const hex of ['#33c57b', '#148c50', '#ff0000', '#808080']) {
      const value = parseInt(hex.slice(1), 16)
      const { h, s, l } = rgbToHSL(
        value >> 16,
        (value >> 8) & 0xff,
        value & 0xff,
      )
      expect(hslToHex(h, s, l)).toBe(hex)
    }
  })
})
