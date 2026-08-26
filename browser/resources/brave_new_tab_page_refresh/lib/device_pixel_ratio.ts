/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// How many screen pixels one CSS pixel is drawn with.
//
// The board needs it because a tile icon must not be scaled up, and "not
// scaled up" is a statement about screen pixels: at 150% scaling a 56px slot
// is 84 real ones, so a 64px icon filling it is stretched even though the CSS
// arithmetic says it fits.
//
// The value changes when the window moves to a display with different scaling,
// and there is no event for it - the idiom is to watch a media query for the
// current value and re-subscribe once it stops matching.
export function useDevicePixelRatio() {
  const [ratio, setRatio] = React.useState(() => window.devicePixelRatio || 1)

  React.useEffect(() => {
    if (typeof window.matchMedia !== 'function') {
      return
    }
    const query = window.matchMedia(`(resolution: ${ratio}dppx)`)
    const onChange = () => setRatio(window.devicePixelRatio || 1)
    query.addEventListener('change', onChange)
    return () => query.removeEventListener('change', onChange)
  }, [ratio])

  return ratio
}
