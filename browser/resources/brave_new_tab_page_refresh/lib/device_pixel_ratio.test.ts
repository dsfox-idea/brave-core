/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { act, renderHook } from '@testing-library/react'

import { useDevicePixelRatio } from './device_pixel_ratio'

// jsdom has no matchMedia and no notion of a display, so the test brings the
// smallest stand-in that still exercises what the hook depends on: the query
// carries the ratio it was subscribed at, and a change event is what tells it
// to look again.
function stubDisplay(initialRatio: number) {
  const listeners = new Set<() => void>()
  const queries: string[] = []
  Object.defineProperty(window, 'devicePixelRatio', {
    configurable: true,
    writable: true,
    value: initialRatio,
  })
  ;(window as any).matchMedia = (query: string) => {
    queries.push(query)
    return {
      addEventListener: (_: string, fn: () => void) => listeners.add(fn),
      removeEventListener: (_: string, fn: () => void) => listeners.delete(fn),
    }
  }
  return {
    queries,
    moveToDisplay(ratio: number) {
      ;(window as any).devicePixelRatio = ratio
      for (const fn of [...listeners]) {
        fn()
      }
    },
    listenerCount: () => listeners.size,
  }
}

describe('device_pixel_ratio', () => {
  it('reports the current ratio', () => {
    stubDisplay(1.5)
    const { result } = renderHook(() => useDevicePixelRatio())
    expect(result.current).toBe(1.5)
  })

  it('watches for the ratio it currently holds', () => {
    const display = stubDisplay(2)
    renderHook(() => useDevicePixelRatio())
    expect(display.queries[0]).toBe('(resolution: 2dppx)')
  })

  it('follows the window to another display', () => {
    const display = stubDisplay(1)
    const { result } = renderHook(() => useDevicePixelRatio())
    act(() => display.moveToDisplay(1.5))
    expect(result.current).toBe(1.5)
    // And it is now watching for the new value, not the old one.
    expect(display.queries.at(-1)).toBe('(resolution: 1.5dppx)')
  })

  it('lets go of its listener when unmounted', () => {
    const display = stubDisplay(1)
    const { unmount } = renderHook(() => useDevicePixelRatio())
    expect(display.listenerCount()).toBe(1)
    unmount()
    expect(display.listenerCount()).toBe(0)
  })

  it('falls back to 1 where the browser reports nothing', () => {
    stubDisplay(0)
    const { result } = renderHook(() => useDevicePixelRatio())
    expect(result.current).toBe(1)
  })
})
