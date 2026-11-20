/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

// Note there are no backticks in these comments: the stylesheet is a
// template literal, and one would end it (lesson 61).
export const style = scoped.css`
  & {
    align-self: stretch;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    /* The clock hangs 56px under the search row (app.style.ts) and is
     * about 142px tall, so the widgets start 16px below it. */
    margin-block-start: calc(var(--search-expand-travel) + 198px);
    padding-block-end: 32px;
  }

  .widget {
    width: min(100%, 720px);
    border-radius: 12px;
    background: ${color.material.thin};
    backdrop-filter: blur(50px);
    color: var(--ntp-foreground, #fff);
    padding: 12px 0;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  h4 {
    font: ${font.default.semibold};
    padding-inline: 16px;
  }

  iframe {
    display: block;
    width: 100%;
    border: none;
    color-scheme: normal;
  }

  form {
    display: flex;
    gap: 8px;
    padding-inline: 16px;
  }

  input {
    flex: 1 1 auto;
    min-width: 0;
    font: ${font.default.regular};
    color: inherit;
    background: rgba(255, 255, 255, 0.12);
    border: 1px solid rgba(255, 255, 255, 0.2);
    border-radius: 8px;
    padding: 6px 10px;
  }

  input:disabled {
    opacity: 0.5;
    cursor: progress;
  }

  .answer-slot {
    min-height: 120px;
    display: flex;
    align-items: center;
    justify-content: center;
    --leo-progressring-size: 32px;
  }

  .error {
    padding-inline: 16px;
    font: ${font.small.regular};
    color: ${color.systemfeedback.errorText};
  }
`
