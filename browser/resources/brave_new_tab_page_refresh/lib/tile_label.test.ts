/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { hostLabel, tileLabel } from './tile_label'

describe('tile_label', () => {
  it('keeps a short name as it is', () => {
    expect(tileLabel('GitHub', 'https://github.com/')).toBe('GitHub')
    expect(tileLabel('Авито', 'https://avito.ru/')).toBe('Авито')
  })

  it('drops the site name bolted on after a dash', () => {
    expect(tileLabel('Почта — Яндекс', 'https://mail.yandex.ru/')).toBe('Почта')
    expect(tileLabel('Inbox - Gmail', 'https://mail.google.com/')).toBe('Inbox')
    expect(tileLabel('Docs | Cloudflare', 'https://cloudflare.com/')).toBe(
      'Docs',
    )
  })

  it('drops a notification count', () => {
    expect(tileLabel('(12) Gmail', 'https://mail.google.com/')).toBe('Gmail')
    expect(tileLabel('[3] Slack', 'https://slack.com/')).toBe('Slack')
  })

  it('does not split on a separator that opens the title', () => {
    expect(tileLabel('- Начало', 'https://example.com/')).toBe('- Начало')
  })

  it('falls back to the host when there is no usable title', () => {
    expect(tileLabel('', 'https://www.example.com/page')).toBe('example.com')
    expect(tileLabel('https://x.example/', 'https://x.example/')).toBe(
      'x.example',
    )
  })

  it('returns an empty label rather than throwing on a bad url', () => {
    expect(tileLabel('', 'not a url')).toBe('')
    expect(hostLabel('not a url')).toBe('')
  })

  it('truncates a long title on a word boundary', () => {
    expect(
      tileLabel('Build and sell your solutions', 'https://github.com/'),
    ).toBe('Build and sell your…')
  })

  it('truncates mid-word when no boundary is near the end', () => {
    const label = tileLabel('Averyveryverylongsinglewordtitle', 'https://x.y/')
    expect(label).toBe('Averyveryverylongsingl…')
    expect(label.length).toBeLessThanOrEqual(23)
  })
})
