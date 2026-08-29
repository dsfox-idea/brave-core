/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

// growser: the NTP cookie-encryption opt-in promo. A single, dismissible card
// shown above the widget stacks while cookie encryption is off and the user has
// not dismissed it. Scoped so it does not leak into the rest of the NTP.
export const style = scoped.css`
  & {
    position: relative;
    display: flex;
    gap: 12px;
    align-items: flex-start;
    padding: 16px 18px;
    border-radius: 16px;
    background: ${color.material.thin};
    backdrop-filter: blur(50px);
    color: ${color.text.primary};
    max-width: 560px;
    margin: 0 auto;
  }

  .title {
    font-weight: 600;
    margin-bottom: 6px;
  }

  .text {
    font-size: 14px;
    opacity: 0.8;
    line-height: 1.45;
  }

  // Emphasized keychain instruction: the OS WILL prompt, and the user must
  // always allow. Visually set apart from the body so it is not missed.
  .callout {
    margin-top: 12px;
    padding: 10px 12px;
    border-radius: 10px;
    background: rgba(30, 163, 98, 0.14);
    border: 1px solid rgba(47, 208, 127, 0.34);
    font-size: 14px;
    font-weight: 600;
    line-height: 1.45;
  }

  .restart {
    font-size: 14px;
    opacity: 0.8;
  }

  .actions {
    display: flex;
    gap: 12px;
    align-items: center;
    margin-top: 14px;
  }

  .dismiss {
    background: none;
    border: none;
    cursor: pointer;
    color: ${color.text.primary};
    opacity: 0.6;
    font-size: 14px;
  }

  .dismiss:hover {
    opacity: 1;
  }
`
