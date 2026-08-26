/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { faviconURL, tileIconURL } from './favicon_url'

describe('favicon_url', () => {
  it('asks the tile source for no particular size', () => {
    // A size would be an invitation to scale up, which is the whole reason
    // this source exists rather than chrome://favicon2.
    expect(tileIconURL('https://example.com/')).toBe(
      'chrome://growser-tile-icon/?pageUrl=https%3A%2F%2Fexample.com%2F',
    )
    expect(tileIconURL('https://example.com/')).not.toContain('size')
  })

  it('escapes a url that would otherwise cut the query short', () => {
    const url = tileIconURL('https://example.com/?a=1&b=2')
    expect(url.split('?').length).toBe(2)
    expect(url).toContain('%26')
  })

  it('still asks favicon2 for the small icon used elsewhere', () => {
    expect(faviconURL('https://example.com/')).toContain('size=64')
  })
})
