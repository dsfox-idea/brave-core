// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Storybook's stand-in for `@brave/leo/react/icon` (see the alias in
// `.storybook/webpack.config.ts`). It re-exports the real module, but pins the
// icon base path to the icons Storybook serves statically and makes
// `setIconBasePath` a no-op for everyone else.
//
// This is needed because Nala keeps the icon base path in a single
// module-level Svelte store which is read at render time, so the *last*
// `setIconBasePath` call wins and retroactively re-renders every mounted icon.
// Lots of production entry points call it at module scope with a `chrome://`
// path, which can't be fetched from the web, so a story that transitively
// imported one of those modules would break icons for the whole page.
//
// Note: the real module is imported by its `.js` path, as the extensionless
// specifier is what's aliased here, and importing it would be circular.
import { setIconBasePath as setLeoIconBasePath } from '@brave/leo/react/icon.js'

// The storybook might be hosted at the root, but it might also be hosted
// somewhere deep. The icons are served from `icons/` relative to the storybook
// (see `staticDirs` in `.storybook/main.ts`), so work out the relative path
// we're at and give that to Nala.
const { pathname } = document.location
if (pathname.endsWith('/iframe.html')) {
  const storybookPath = pathname.substring(0, pathname.lastIndexOf('/'))
  setLeoIconBasePath(`${storybookPath}/icons`)
} else {
  // Perhaps storybook was upgraded and this changed?
  console.error(
    'Could not ascertain path that the storybook is hosted at. Not able to '
      + 'set static icon path!',
  )
}

export { default } from '@brave/leo/react/icon.js'
export * from '@brave/leo/react/icon.js'

/**
 * A no-op stand-in for Nala's `setIconBasePath`. An explicit export shadows the
 * `export *` above, so consumers get this rather than Nala's implementation.
 * Storybook always serves icons as static files, so the base path is fixed by
 * this module and must not be overridden.
 */
export const setIconBasePath = (_basePath: string) => {}
