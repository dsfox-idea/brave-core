/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export function faviconURL(url: string) {
  return 'chrome://favicon2/?size=64&pageUrl=' + encodeURIComponent(url)
}

// The icon for a top sites tile: the largest one the site gave us, at the
// resolution it was stored in. chrome://favicon2 resizes whatever it has up to
// the size asked for, which on a tile turns a 32 pixel favicon into a smudge;
// this source never scales anything up. See tile_icon_source.h.
export function tileIconURL(url: string) {
  return 'chrome://growser-tile-icon/?pageUrl=' + encodeURIComponent(url)
}
