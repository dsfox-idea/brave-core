/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { mangle, mangleAll } from 'lit_mangler'

// Growser-154: this is the page a user sees at growser://about - Chromium 152
// serves the list from the chrome_urls WebUI, and this mangler is what brands
// it. The older brave/components/webui/about_ui.cc does the same job for the
// page that used to be there; editing that one alone changed nothing, which is
// how this file was found.

// Rewrite H2 text content
mangle((element) => {
  const h2 = element.querySelector('h2')
  if (!h2) {
    throw new Error('[chrome_urls override] Missing H2 element')
  }
  h2.textContent = 'List of Growser URLs'
})

// Rewrite chrome://chrome-urls -> growser://chrome-urls
mangle((element) => {
  const anchor = element.querySelector('a')
  if (!anchor) {
    throw new Error('[chrome_urls override] Missing anchor element')
  }
  if (anchor.textContent !== 'chrome://chrome-urls') {
    throw new Error('[chrome_urls override] Unexpected anchor textContent')
  }
  anchor.textContent = 'growser://chrome-urls'
}, x => x.text.includes('href="#"'))

// Rewrite standard chrome URLs to use growser: scheme (these appear under the
// "List of Growser URLs" header) and rewrite internal debugging page URLs to use
// growser: scheme (these appear under the "Internal Debugging Page URLs" header
// when the debugging pages are enabled)
mangleAll((element) => {
  const anchor = element.querySelector('a')
  if (!anchor) {
    throw new Error('[chrome_urls override] Missing anchor element')
  }
  if (anchor.textContent !== '\${info.url}') {
    throw new Error('[chrome_urls override] Unexpected anchor textContent')
  }
  anchor.textContent = '\${info.url.replace(/chrome:/, "growser:")}'
}, x => x.text.includes('href="${info.url}"'))

// Rewrite inactive chrome URLs to use growser: scheme (these also appear under
// the "List of Growser URLs" header) and rewrite internal debugging page URLs to
// use growser: scheme (these appear under the "Internal Debugging Page URLs"
// header when the debugging pages are disabled)
mangleAll((element) => {
  const listItem = element.querySelector('li')
  if (!listItem) {
    throw new Error('[chrome_urls override] Missing list item element')
  }
  if (listItem.textContent !== '\${info.url}') {
    throw new Error('[chrome_urls override] Unexpected list item textContent')
  }
  listItem.textContent = '\${info.url.replace(/chrome:/, "growser:")}'
}, x => x.text.includes('<li>${info.url}</li>'))

// Rewrite command URLs to use growser: scheme (these appear under the
// "Command URLs for Debug" header)
mangle((element) => {
  const listItem = element.querySelector('li')
  if (!listItem) {
    throw new Error('[chrome_urls override] Missing list item element')
  }
  if (listItem.textContent !== '\${url}') {
    throw new Error('[chrome_urls override] Unexpected list item textContent')
  }
  listItem.textContent = '\${url.replace(/chrome:/, "growser:")}'
}, x => x.text.includes('<li>${url}</li>'))
