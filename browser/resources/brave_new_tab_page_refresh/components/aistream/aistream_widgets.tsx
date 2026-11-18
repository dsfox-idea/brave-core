/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser#422: widgets a local agent draws. The page never talks to the
// network: the browser process holds the bridge connection (WidgetHandler),
// and the agent's markup is shown only inside the chrome-untrusted:// frame,
// which runs no script of the agent's and loads nothing.

import * as React from 'react'

import * as mojom from 'gen/brave/browser/aistream/aistream.mojom.m.js'

import { style } from './aistream_widgets.style'

const frameUrl = 'chrome-untrusted://aistream-widget/'
const frameOrigin = 'chrome-untrusted://aistream-widget'

let proxy: {
  handler: mojom.WidgetHandlerRemote
  router: mojom.WidgetPageCallbackRouter
} | null = null

function getProxy() {
  if (!proxy) {
    const router = new mojom.WidgetPageCallbackRouter()
    const handler = mojom.WidgetHandler.getRemote()
    handler.setPage(router.$.bindNewPipeAndPassRemote())
    proxy = { handler, router }
  }
  return proxy
}

// The page as the agent sees it, without the widgets' own frames' innards
// (an iframe's outerHTML carries no document).
function snapshotDom() {
  return document.documentElement.outerHTML
}

export function AistreamWidgets() {
  const [widgets, setWidgets] = React.useState<mojom.Widget[]>([])

  const updateMarkup = React.useCallback((widgetId: string, markup: string) => {
    setWidgets((current) =>
      current.map((widget) =>
        widget.id === widgetId ? { ...widget, markup } : widget,
      ),
    )
  }, [])

  React.useEffect(() => {
    const { handler, router } = getProxy()
    handler
      .getWidgets()
      .then((reply: { widgets: mojom.Widget[] }) => setWidgets(reply.widgets))
    const listenerId = router.onWidgetMarkup.addListener(updateMarkup)
    return () => {
      router.removeListener(listenerId)
    }
  }, [updateMarkup])

  if (widgets.length === 0) {
    return null
  }
  return (
    <div data-css-scope={style.scope} data-testid='aistream-widgets'>
      {widgets.map((widget) => (
        <AistreamWidget
          key={widget.id}
          widget={widget}
          onMarkup={updateMarkup}
        />
      ))}
    </div>
  )
}

interface WidgetProps {
  widget: mojom.Widget
  onMarkup: (widgetId: string, markup: string) => void
}

function AistreamWidget(props: WidgetProps) {
  const frameRef = React.useRef<HTMLIFrameElement>(null)
  const [frameReady, setFrameReady] = React.useState(false)
  const [height, setHeight] = React.useState(0)
  const [message, setMessage] = React.useState('')
  const [pending, setPending] = React.useState(false)
  const [error, setError] = React.useState('')

  React.useEffect(() => {
    function listener(event: MessageEvent) {
      if (event.origin !== frameOrigin) {
        return
      }
      if (!event.source || event.source !== frameRef.current?.contentWindow) {
        return
      }
      if (event.data?.type === 'ready') {
        setFrameReady(true)
      } else if (event.data?.type === 'height') {
        setHeight(Math.ceil(Number(event.data.height) || 0))
      }
    }
    window.addEventListener('message', listener)
    return () => window.removeEventListener('message', listener)
  }, [])

  React.useEffect(() => {
    if (!frameReady) {
      return
    }
    frameRef.current?.contentWindow?.postMessage(
      { type: 'markup', markup: props.widget.markup },
      frameOrigin,
    )
  }, [frameReady, props.widget.markup])

  async function send() {
    const text = message.trim()
    if (!text || pending) {
      return
    }
    setPending(true)
    setError('')
    const reply = await getProxy().handler.sendMessage(
      props.widget.id,
      text,
      snapshotDom(),
    )
    setPending(false)
    if (reply.error) {
      setError(reply.error)
    } else {
      // The bridge also streams this, but the answer must not depend on the
      // stream being up.
      props.onMarkup(props.widget.id, reply.markup)
      setMessage('')
    }
  }

  return (
    <section className='widget' data-widget-id={props.widget.id}>
      {props.widget.title && <h4>{props.widget.title}</h4>}
      <iframe
        ref={frameRef}
        src={frameUrl}
        title={props.widget.title}
        sandbox='allow-scripts allow-same-origin'
        style={{ height: `${height}px` }}
      />
      <form
        onSubmit={(event) => {
          event.preventDefault()
          send()
        }}
      >
        <input
          type='text'
          value={message}
          disabled={pending}
          onChange={(event) => setMessage(event.target.value)}
        />
        <button type='submit' disabled={pending || !message.trim()}>
          {pending ? '…' : '↵'}
        </button>
      </form>
      {error && <p className='error'>{error}</p>}
    </section>
  )
}
