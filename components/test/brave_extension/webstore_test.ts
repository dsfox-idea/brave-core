/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser#366: the Chrome Web Store button names this browser. The labels are
// read from the extension's own _locales, so the test runs the shipped text.

import * as fs from 'fs'
import * as path from 'path'

const LOCALES = path.join(__dirname,
  '../../brave_extension/extension/brave_extension/_locales')

const messagesFor = (locale: string): Record<string, string> => {
  const raw = JSON.parse(
    fs.readFileSync(path.join(LOCALES, locale, 'messages.json'), 'utf8'))
  return Object.fromEntries(
    Object.entries(raw).map(([key, value]: [string, any]) => [key, value.message]))
}

const relabel = async (locale: string, html: string) => {
  const messages = messagesFor(locale)
  jest.spyOn(chrome.i18n, 'getMessage')
    .mockImplementation((key: string) => messages[key] || '')
  document.body.innerHTML = html
  jest.isolateModules(() => {
    require('../../brave_extension/extension/brave_extension/webstore')
  })
  document.body.appendChild(document.createElement('i'))
  await new Promise((resolve) => setTimeout(resolve, 0))
  return messages
}

describe('Web Store button', () => {
  afterEach(() => {
    jest.restoreAllMocks()
    document.body.innerHTML = ''
  })

  it.each(['en_US', 'ru'])('reads "Growser" in %s', async (locale) => {
    const messages = messagesFor(locale)
    await relabel(locale,
      `<button><span jsname="a">${messages.addToChrome}</span></button>` +
      `<button><span jsname="b">${messages.removeFromChrome}</span></button>`)
    const labels = [...document.querySelectorAll('span')].map((s) => s.textContent)
    expect(labels).toEqual([
      messages.addToBrave.replace('Brave', 'Growser'),
      messages.removeFromBrave.replace('Brave', 'Growser'),
    ])
    expect(labels.join(' ')).not.toMatch(/\bBrave\b/)
  })

  it('leaves other buttons alone', async () => {
    await relabel('en_US', '<button><span jsname="a">Share</span></button>')
    expect(document.querySelector('span')!.textContent).toBe('Share')
  })
})
