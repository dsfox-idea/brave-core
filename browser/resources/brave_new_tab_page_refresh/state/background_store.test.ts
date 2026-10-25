/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BraveBackground,
  defaultBackgroundStore,
  getCurrentBackground,
  SelectedBackgroundType,
  solidThemeBackground,
} from './background_store'

// growser#392: the settings toggle turns the background off, and "off" is one
// flat colour that follows the theme.

function stateWith(backgroundsEnabled: boolean) {
  const photo = { imageUrl: 'chrome://background-wallpaper/a.jpg' }
  return {
    ...defaultBackgroundStore().getState(),
    initialized: true,
    backgroundsEnabled,
    braveBackgrounds: [photo as BraveBackground],
    selectedBackground: { type: SelectedBackgroundType.kBrave, value: '' },
  }
}

describe('getCurrentBackground', () => {
  it('draws the photo while backgrounds are on', () => {
    expect(getCurrentBackground(stateWith(true))).toMatchObject({
      type: 'brave',
      imageUrl: 'chrome://background-wallpaper/a.jpg',
    })
  })

  it('draws the flat theme colour when backgrounds are off', () => {
    expect(getCurrentBackground(stateWith(false))).toEqual({
      type: 'color',
      cssValue: solidThemeBackground,
    })
  })

  it('picks dark grey for the dark theme and near-white for the light one', () => {
    expect(solidThemeBackground).toBe('light-dark(#f4f4f5, #202124)')
  })
})
