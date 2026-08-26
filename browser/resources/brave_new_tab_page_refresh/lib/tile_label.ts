/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The caption a tile shows on hover. A page title is written for a tab strip,
// not for a tile: it carries an unread count at the front and the site's name
// after a dash at the end. What is wanted is the short name a person would say
// out loud, so take the first segment and cut it to one line.

const maxLength = 22

// The separators a title uses to bolt the site name onto the page name. Written
// as a list rather than one expression so each is visible.
const separators = [' — ', ' – ', ' - ', ' | ', ' · ', ' :: ', ' » ']

// "(3) Inbox" and "[2] Chat" - a notification count, never part of the name.
const countPrefix = /^[([][0-9]+[)\]][\s]*/

export function hostLabel(url: string) {
  try {
    const host = new URL(url).hostname
    return host.startsWith('www.') ? host.slice(4) : host
  } catch {
    return ''
  }
}

export function tileLabel(title: string, url: string) {
  let label = title.replace(countPrefix, '').trim()

  for (const separator of separators) {
    const index = label.indexOf(separator)
    // Only when what precedes it is a name in its own right: "A - B" splits,
    // "X - " does not, and a title that opens with the separator is left alone.
    if (index >= 2) {
      label = label.slice(0, index).trim()
      break
    }
  }

  // A title that is just the URL (which is what the browser sends when a page
  // has no title at all) reads better as a bare host.
  if (!label || label === url) {
    label = hostLabel(url)
  }

  if (label.length <= maxLength) {
    return label
  }
  // Cut on a word boundary when one is near the end, so a name is not sliced
  // mid-word for the sake of two characters.
  const cut = label.slice(0, maxLength)
  const space = cut.lastIndexOf(' ')
  return (space >= maxLength - 6 ? cut.slice(0, space) : cut).trimEnd() + '…'
}
