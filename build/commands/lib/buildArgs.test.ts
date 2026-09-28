// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getBuildArgs } from './buildArgs.ts'

// A Config stand-in: the named fields are the ones this test is about. Every
// predicate (isX, shouldX, ...) answers false, the two methods that act on
// the args do nothing, and every other field is undefined.
function fakeConfig(fields: Record<string, unknown>): any {
  return new Proxy(fields, {
    get(target, prop: string) {
      if (prop in target) {
        return target[prop]
      }
      if (/^(is|should|use|enable|has)[A-Z]/.test(prop)) {
        return () => false
      }
      if (/^forward/.test(prop)) {
        return () => undefined
      }
      return undefined
    },
  })
}

function officialArgs(targetOS: string) {
  return getBuildArgs(
    fakeConfig({
      targetOS,
      targetArch: 'arm64',
      srcDir: '/nonexistent',
      isOfficialBuild: () => true,
      isBraveReleaseBuild: () => false,
    }),
  )
}

describe('getBuildArgs dSYMs in an official non-release build (growser#335)', () => {
  const onDarwin = process.platform === 'darwin' ? it : it.skip

  onDarwin('keeps them off on macOS, where only relocatability is at stake', () => {
    expect(officialArgs('mac').enable_dsyms).toBe(false)
  })

  // iOS strips without saving the unstripped copy it declares, so turning
  // dSYMs off there breaks the link of every dylib under siso.
  onDarwin('leaves them to the default on iOS', () => {
    expect(officialArgs('ios').enable_dsyms).toBeUndefined()
  })
})
