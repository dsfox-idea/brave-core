/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser#422: the page's side of the widget bridge, shared by the widgets
// and the search box (which offers to ask the agent). The page never talks to
// the network: the browser process holds the bridge connection.

import * as React from 'react'

import * as mojom from 'gen/brave/browser/aistream/aistream.mojom.m.js'
import { createStateStore } from '$web-common/state_store'

// A question on its way to the agent, or one that failed.
export interface PendingAsk {
  key: string
  query: string
  error: string
}

interface AistreamState {
  on: boolean
  widgets: mojom.Widget[]
  pending: PendingAsk[]
}

class AistreamStore {
  private readonly store = createStateStore<AistreamState>({
    on: false,
    widgets: [],
    pending: [],
  })
  private handler: mojom.WidgetHandlerRemote | null = null
  private askCount = 0

  get state() {
    return this.store.getState()
  }

  addListener(listener: (state: AistreamState) => void) {
    return this.store.addListener(listener)
  }

  // Connects once, on first use.
  connect() {
    if (this.handler) {
      return this.handler
    }
    const router = new mojom.WidgetPageCallbackRouter()
    this.handler = mojom.WidgetHandler.getRemote()
    this.handler.setPage(router.$.bindNewPipeAndPassRemote())
    router.onWidgetMarkup.addListener((widgetId: string, markup: string) =>
      this.updateMarkup(widgetId, markup),
    )
    this.handler.isOn().then(({ on }: { on: boolean }) => {
      this.store.update({ on })
    })
    this.reloadWidgets()
    return this.handler
  }

  updateMarkup(widgetId: string, markup: string) {
    const { widgets } = this.state
    if (!widgets.some((widget) => widget.id === widgetId)) {
      // A widget the agent added (an answer): the bridge has its place.
      this.reloadWidgets()
      return
    }
    this.store.update({
      widgets: widgets.map((widget) =>
        widget.id === widgetId ? { ...widget, markup } : widget,
      ),
    })
  }

  async ask(query: string) {
    const handler = this.connect()
    const key = `pending-${++this.askCount}`
    this.store.update({
      pending: [{ key, query, error: '' }, ...this.state.pending],
    })
    const reply = await handler.ask(query, snapshotDom())
    if (reply.error) {
      this.store.update({
        pending: this.state.pending.map((entry) =>
          entry.key === key ? { ...entry, error: reply.error } : entry,
        ),
      })
      return
    }
    // The bridge's list is the order and the limit; the stream may be down,
    // so the answer does not wait for it.
    await this.reloadWidgets()
    this.store.update({
      pending: this.state.pending.filter((entry) => entry.key !== key),
    })
  }

  private async reloadWidgets() {
    const { widgets } = await this.handler!.getWidgets()
    this.store.update({ widgets })
  }
}

export const aistream = new AistreamStore()

export function useAistream<T>(map: (state: AistreamState) => T): T {
  const [value, setValue] = React.useState(() => map(aistream.state))
  React.useEffect(() => {
    aistream.connect()
    setValue(map(aistream.state))
    return aistream.addListener((state) => setValue(map(state)))
  }, [])
  return value
}

// The page as the agent sees it, without the widgets' own frames' innards
// (an iframe's outerHTML carries no document).
export function snapshotDom() {
  return document.documentElement.outerHTML
}
