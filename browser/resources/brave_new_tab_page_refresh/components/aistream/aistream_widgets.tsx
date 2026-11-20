/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser#422: widgets a local agent draws. The page never talks to the
// network: the browser process holds the bridge connection (WidgetHandler),
// and the agent's markup is shown only inside the chrome-untrusted:// frame,
// which runs no script of the agent's and loads nothing.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

import * as mojom from 'gen/brave/browser/aistream/aistream.mojom.m.js'

import { aistream, snapshotDom, useAistream } from './aistream_store'
import { style } from './aistream_widgets.style'

const frameUrl = 'chrome-untrusted://aistream-widget/'
const frameOrigin = 'chrome-untrusted://aistream-widget'

// The place the answer will take, held while the agent thinks.
function AnswerSlot() {
  return (
    <div className='answer-slot' data-testid='aistream-waiting'>
      <ProgressRing />
    </div>
  )
}

export function AistreamWidgets() {
  const widgets = useAistream((s) => s.widgets)
  const pending = useAistream((s) => s.pending)

  if (widgets.length === 0 && pending.length === 0) {
    return null
  }
  return (
    <div data-css-scope={style.scope} data-testid='aistream-widgets'>
      {pending.map((entry) => (
        <section className='widget pending' key={entry.key}>
          <h4>{entry.query}</h4>
          {entry.error ? <p className='error'>{entry.error}</p> : <AnswerSlot />}
        </section>
      ))}
      {widgets.map((widget) => (
        <AistreamWidget
          key={widget.id}
          widget={widget}
        />
      ))}
    </div>
  )
}

interface WidgetProps {
  widget: mojom.Widget
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
    const reply = await aistream.connect().sendMessage(
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
      aistream.updateMarkup(props.widget.id, reply.markup)
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
        style={{
          height: `${height}px`,
          display: props.widget.markup && !pending ? undefined : 'none',
        }}
      />
      {pending && <AnswerSlot />}
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
        {/* Enter submits; there is no button to press. */}
      </form>
      {error && <p className='error'>{error}</p>}
    </section>
  )
}
