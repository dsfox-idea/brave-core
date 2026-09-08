/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  drawingSource,
  installPack,
  lookupPackedIcon,
  packKeyFor,
  packedDomainCount,
  packedRevision,
} from './icon_pack'

describe('icon_pack', () => {
  it('holds the sites a board is likely to carry', () => {
    // A guard on the generated module rather than on a number: an empty pack
    // would make every tile fall back and nothing would look broken.
    expect(packedDomainCount()).toBeGreaterThan(300)
  })

  it('keys a url by its host, without the www some sites use', () => {
    expect(packKeyFor('https://www.example.com/deep/page?a=1')).toBe(
      'example.com',
    )
    expect(packKeyFor('http://example.com')).toBe('example.com')
    expect(packKeyFor('not a url')).toBe('')
  })

  it('finds a brand the pack was built with', () => {
    const icon = lookupPackedIcon('https://github.com/')
    expect(icon).not.toBeNull()
    expect(icon!.kind).toBe('mono')
    expect(icon!.colour).toMatch(/^#[0-9a-f]{6}$/)
    expect(icon!.drawing).toContain('<svg')
  })

  it('gives one drawing to every domain of a brand', () => {
    // Several domains of the selection are one brand - nineteen of them are
    // Google - and the pack holds one mark for each brand, not one per
    // domain.
    const org = lookupPackedIcon('https://telegram.org/')
    const short = lookupPackedIcon('https://t.me/')
    expect(org).not.toBeNull()
    expect(short).not.toBeNull()
    expect(short!.drawing).toBe(org!.drawing)
    expect(short!.colour).toBe(org!.colour)
  })

  it('falls back to the registrable parent for a section of a site', () => {
    const icon = lookupPackedIcon('https://news.github.com/')
    expect(icon).not.toBeNull()
    expect(icon!.drawing).toBe(lookupPackedIcon('https://github.com/')!.drawing)
  })

  // growser#175: the fallback above is right for a section of a site and
  // wrong for a separate product. Three services of one ecosystem were
  // drawing the same red Yandex tile, which a person cannot tell apart
  // without reading the url - the one thing a tile exists to save them.
  // Nothing in the lookup had to change: a service with a mark of its own is
  // an entry of its own, and an exact host already outranks its parent.
  it('gives a service with its own mark a tile of its own', () => {
    const parent = lookupPackedIcon('https://yandex.ru/')
    for (const service of ['https://music.yandex.ru/',
                           'https://mail.yandex.ru/']) {
      const icon = lookupPackedIcon(service)
      expect(icon).not.toBeNull()
      expect(icon!.drawing).not.toBeNull()
      expect(icon!.drawing).not.toBe(parent!.drawing)
    }
  })

  it('knows nothing about the rest of the web, and says so', () => {
    expect(lookupPackedIcon('https://an-intranet.local/')).toBeNull()
    expect(lookupPackedIcon('file:///c:/notes.txt')).toBeNull()
    expect(lookupPackedIcon('')).toBeNull()
  })

  it('hands a drawing over as a data url, never as markup', () => {
    // The page enforces Trusted Types: assigning an SVG to innerHTML throws
    // and takes the whole board down with it, which is how this was found.
    const source = drawingSource('<svg><path fill="currentColor"/></svg>', true)
    expect(source.startsWith('data:image/svg+xml')).toBe(true)
    expect(source).not.toContain('<svg')
  })

  it('writes the white into a single-colour mark', () => {
    // An image is a document of its own and cannot inherit a colour from the
    // page, so `currentColor` has to be resolved before it is handed over.
    // Read in its encoded form: this suite's own setup replaces
    // decodeURIComponent with a stub that answers 'test' to everything.
    const mono = drawingSource('<svg><path fill="currentColor"/></svg>', true)
    expect(mono).toContain('%23ffffff')
    expect(mono).not.toContain('currentColor')

    const full = drawingSource('<svg><path fill="#ff0000"/></svg>', false)
    expect(full).toContain('%23ff0000')
  })

  it('paints every tile in a colour a white mark can sit on', () => {
    // The pack is built with a 3:1 floor against white; a drawing that fails
    // it would be invisible on its own tile.
    for (const url of ['https://github.com/', 'https://dhl.de/',
                       'https://snapchat.com/', 'https://spotify.com/']) {
      const icon = lookupPackedIcon(url)
      if (!icon) {
        continue
      }
      const value = parseInt(icon.colour.slice(1), 16)
      const channel = (raw: number) => {
        const part = raw / 255
        return part <= 0.03928
          ? part / 12.92
          : Math.pow((part + 0.055) / 1.055, 2.4)
      }
      const luminance =
        0.2126 * channel(value >> 16)
        + 0.7152 * channel((value >> 8) & 0xff)
        + 0.0722 * channel(value & 0xff)
      expect(1.05 / (luminance + 0.05)).toBeGreaterThanOrEqual(2.9)
    }
  })

  // growser#190: a pack the browser hands over replaces the bundled one. The
  // bundled pack is the floor, so every way a payload can be wrong has to
  // leave the board exactly as it was - a half-read pack draws blank tiles,
  // which is worse than an older pack drawn correctly.
  describe('a pack from the browser', () => {
    const good = JSON.stringify({
      format: 1,
      domains: { 'example.com': 'brand' },
      brands: { brand: { kind: 'full', colour: '#123456', name: 'Brand' } },
      drawings: { brand: '<svg/>' },
    })

    it('replaces the bundled tables', () => {
      expect(lookupPackedIcon('https://example.com/')).toBeNull()
      expect(installPack(good)).toBe(true)
      const icon = lookupPackedIcon('https://example.com/')
      expect(icon).not.toBeNull()
      expect(icon!.name).toBe('Brand')
      expect(packedRevision()).toBeGreaterThan(0)
    })

    it('refuses anything it does not understand, and changes nothing', () => {
      const before = packedRevision()
      const refused = [
        '',
        'not json',
        JSON.stringify({ format: 2, domains: {}, brands: {}, drawings: {} }),
        JSON.stringify({ format: 1, domains: {}, brands: {} }),
        JSON.stringify({ format: 1, domains: [], brands: {}, drawings: {} }),
      ]
      for (const payload of refused) {
        expect(installPack(payload)).toBe(false)
      }
      // Not one of them moved the pack, which is the claim that matters.
      expect(packedRevision()).toBe(before)
    })
  })
})
