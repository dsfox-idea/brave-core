/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { getString } from '../../lib/strings'
import { NewTabPageProxy } from '../../state/new_tab_page_proxy'

import { style } from './encryption_promo_widget.style'

// growser: a dismissible NTP card that offers to turn on cookie encryption
// (opt-in, see kGrowserCookieEncryptionEnabled). Shown only while encryption is
// off and the promo has not been dismissed. Enabling flips the pref; the cookie
// store is recreated with OSCrypt on the next browser restart, so the card asks
// the user to restart. Dismissing hides the card for good via a separate pref.
export function EncryptionPromoWidget() {
  // null until the pref state has loaded, so the card does not flash before
  // being hidden.
  const [enabled, setEnabled] = React.useState<boolean | null>(null)
  const [dismissed, setDismissed] = React.useState(false)
  const [justEnabled, setJustEnabled] = React.useState(false)

  React.useEffect(() => {
    (async () => {
      const { enabled, dismissed } =
        await NewTabPageProxy.getInstance().handler.getCookieEncryptionState()
      setEnabled(enabled)
      setDismissed(dismissed)
    })()
  }, [])

  if (enabled === null || enabled || dismissed) {
    return null
  }

  async function onEnable() {
    await NewTabPageProxy.getInstance().handler.setCookieEncryptionEnabled(true)
    setJustEnabled(true)
  }

  async function onDismiss() {
    await NewTabPageProxy.getInstance()
        .handler.setCookieEncryptionPromoDismissed(true)
    setDismissed(true)
  }

  return (
    <div data-css-scope={style.scope} className='encryption-promo'>
      <Icon name='shield-done' />
      <div className='body'>
        <div className='title'>
          {getString(S.NEW_TAB_ENCRYPTION_PROMO_TITLE)}
        </div>
        {justEnabled ? (
          <div className='restart'>
            {getString(S.NEW_TAB_ENCRYPTION_PROMO_RESTART)}
          </div>
        ) : (
          <>
            <div className='text'>
              {getString(S.NEW_TAB_ENCRYPTION_PROMO_BODY)}
            </div>
            <div className='actions'>
              <Button size='small' onClick={onEnable}>
                {getString(S.NEW_TAB_ENCRYPTION_PROMO_ENABLE)}
              </Button>
              <button className='dismiss' onClick={onDismiss}>
                {getString(S.NEW_TAB_ENCRYPTION_PROMO_DISMISS)}
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  )
}