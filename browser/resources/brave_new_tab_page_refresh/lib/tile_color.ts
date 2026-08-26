/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The colour a top site tile is painted in: the dominant colour of the site's
// favicon, held to a lightness range where a white icon still reads on top.
//
// The bounds are Chromium's, from `favicon_base::SetDominantColorAsBackground`
// (components/favicon_base/fallback_icon_style.cc), which paints the letter
// tiles shown when a site has no icon at all. Same job, same constraint.
const minLightness = 0.15
const maxLightness = 0.67

// A favicon is usually a coloured mark on a white or transparent field. Every
// pixel is weighted by how colourful it is so the field cannot outvote the
// mark, with a floor so an all-grey icon (GitHub, Apple) still yields its own
// grey rather than nothing.
const greyWeight = 0.05

// Colours are bucketed 4 bits per channel before counting, so that shades of
// one colour add up instead of splitting the vote between them.
function bucketOf(r: number, g: number, b: number) {
  return ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4)
}

interface Bucket {
  weight: number
  r: number
  g: number
  b: number
  count: number
}

export function rgbToHSL(r: number, g: number, b: number) {
  r /= 255
  g /= 255
  b /= 255
  const max = Math.max(r, g, b)
  const min = Math.min(r, g, b)
  const l = (max + min) / 2
  const d = max - min
  if (d === 0) {
    return { h: 0, s: 0, l }
  }
  const s = d / (1 - Math.abs(2 * l - 1))
  let h: number
  if (max === r) {
    h = ((g - b) / d) % 6
  } else if (max === g) {
    h = (b - r) / d + 2
  } else {
    h = (r - g) / d + 4
  }
  h *= 60
  return { h: h < 0 ? h + 360 : h, s, l }
}

export function hslToHex(h: number, s: number, l: number) {
  const c = (1 - Math.abs(2 * l - 1)) * s
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1))
  const m = l - c / 2
  const [r, g, b] =
    h < 60
      ? [c, x, 0]
      : h < 120
        ? [x, c, 0]
        : h < 180
          ? [0, c, x]
          : h < 240
            ? [0, x, c]
            : h < 300
              ? [x, 0, c]
              : [c, 0, x]
  const channel = (value: number) =>
    Math.round((value + m) * 255)
      .toString(16)
      .padStart(2, '0')
  return '#' + channel(r) + channel(g) + channel(b)
}

// `pixels` is RGBA, as `CanvasRenderingContext2D.getImageData` returns it.
// Returns null when the icon is entirely transparent, which is what
// chrome://favicon2 hands back for a site whose icon has not been fetched -
// the caller keeps whatever colour it had rather than painting the tile blank.
export function dominantColorOf(pixels: Uint8ClampedArray): string | null {
  const buckets = new Map<number, Bucket>()
  let best: Bucket | null = null

  for (let i = 0; i + 3 < pixels.length; i += 4) {
    if (pixels[i + 3] < 128) {
      continue
    }
    const r = pixels[i]
    const g = pixels[i + 1]
    const b = pixels[i + 2]
    const weight = (Math.max(r, g, b) - Math.min(r, g, b)) / 255 + greyWeight

    const key = bucketOf(r, g, b)
    let bucket = buckets.get(key)
    if (!bucket) {
      bucket = { weight: 0, r: 0, g: 0, b: 0, count: 0 }
      buckets.set(key, bucket)
    }
    bucket.weight += weight
    bucket.r += r
    bucket.g += g
    bucket.b += b
    bucket.count += 1

    if (!best || bucket.weight > best.weight) {
      best = bucket
    }
  }

  if (!best) {
    return null
  }

  // The bucket is 16 shades wide; average the pixels that landed in it rather
  // than taking the bucket's corner.
  const { h, s, l } = rgbToHSL(
    best.r / best.count,
    best.g / best.count,
    best.b / best.count,
  )
  return hslToHex(h, s, Math.min(Math.max(l, minLightness), maxLightness))
}

// Reads an icon's pixels through a canvas. chrome://favicon2 is same-origin
// enough for this - measured, not assumed: the canvas stays readable and
// `getImageData` does not throw. The try/catch is there because a future
// change to that source would turn this into a SecurityError, and a tile
// without a colour is better than a page that fails to render.
export async function readIconPixels(
  src: string,
): Promise<Uint8ClampedArray | null> {
  const image = new Image()
  image.src = src
  try {
    await image.decode()
    const size = Math.min(image.naturalWidth || 32, 64)
    if (size === 0) {
      return null
    }
    const canvas = document.createElement('canvas')
    canvas.width = size
    canvas.height = size
    const context = canvas.getContext('2d', { willReadFrequently: true })
    if (!context) {
      return null
    }
    context.drawImage(image, 0, 0, size, size)
    return context.getImageData(0, 0, size, size).data
  } catch {
    return null
  }
}

export async function tileColorFor(iconURL: string) {
  const pixels = await readIconPixels(iconURL)
  return pixels ? dominantColorOf(pixels) : null
}
