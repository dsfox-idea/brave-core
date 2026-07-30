// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'emptykit.css'
import * as React from 'react'
import { withKnobs, boolean } from '@storybook/addon-knobs'
import { getString } from './locale'
import ThemeProvider from '../components/common/BraveCoreThemeProvider'
import StyledComponentsProvider from '../components/common/StyledComponentsProvider'

// Nala design tokens (the `--leo-*` custom properties). In the browser these
// come from `chrome://resources/brave/css/nala.css`, which Storybook can't
// load, so pull in the static token stylesheet globally here. It defines both
// the light and dark values, keyed off `prefers-color-scheme`.
import '@brave/leo/tokens/css/variables.css'

// Fonts
import '../ui/webui/resources/fonts/poppins.css'
import '../ui/webui/resources/fonts/manrope.css'
import '../ui/webui/resources/fonts/inter.css'

// Point Nala icons at the icons Storybook serves statically. Imported for its
// side effect, so the path is set before any story renders. `@brave/leo/react/
// icon` is aliased to this module, which no-ops `setIconBasePath` so production
// code can't override the path.
import './leo-icon-mock'

export const parameters = {
  backgrounds: {
    default: 'Dynamic',
    values: [
      { name: 'Dynamic', value: 'var(--background1)' },
      { name: 'Neutral300', value: '#DEE2E6' },
      { name: 'Grey700', value: '#5E6175' },
      { name: 'White', value: '#FFF' },
      { name: 'Grey900', value: '#1E2029' },
    ],
  },
}

const global: any = window
global.loadTimeData = {
  getString,
  getBoolean(key: string) {
    return false
  },
  getInteger(key: string) {
    return 0
  },
}

if (!global.chrome) global.chrome = { extension: {} }
global.chrome.extension = {
  inIncognitoContext: false,
}

export default {
  decorators: [
    // Mirror production: restore styled-components v5 DOM prop filtering so
    // custom style-only props don't leak onto DOM nodes as invalid attributes.
    (Story: () => JSX.Element) => (
      <StyledComponentsProvider>
        <Story />
      </StyledComponentsProvider>
    ),
    (Story: () => JSX.Element) => (
      <div dir={boolean('rtl?', false) ? 'rtl' : ''}>
        <Story />
      </div>
    ),
    (Story: () => JSX.Element, context: any) => (
      <ThemeProvider
        dark={context.args.darkTheme}
        light={context.args.lightTheme}
      >
        <Story />
      </ThemeProvider>
    ),
    withKnobs,
  ],
}
