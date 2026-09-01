/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`

  & {
    --self-transition-duration: var(--search-transition-duration, 120ms);

    /* growser (#117): how far the box sits below its slot. Driven by transform
       (composited) rather than inset-block-start (a layout property that reflows
       on every frame - the source of the stutter). The value moved to the page
       root in app.style.ts (#136) because the clock is positioned from it too,
       and one number with two readers must have one home; it inherits down to
       here. Tune it there if the resting spot needs to sit higher or lower. */

    anchor-name: --search-box-anchor;
    color: ${color.text.primary};
    min-height: 48px;
  }

  .search-container {
    position: absolute;
    position-anchor: --search-box-anchor;
    inset: anchor(start) 0 auto;

    display: block;
    margin: 0 auto;
    overflow: visible;
    width: calc(100vw - 32px);
    max-width: 416px;

    /* The box LIVES at its expanded height: it used to ride down
       --search-expand-travel on focus, and the owner asked for the resting
       box to sit where the click would have taken it. Only the width still
       animates. */
    translate: 0 var(--search-expand-travel);

    transition-property: overlay, max-width;
    transition-duration: var(--self-transition-duration);
    transition-timing-function: ease-out;
    transition-behavior: allow-discrete;

    /* growser (#117): the dim toggles instantly instead of fading. Fading the
       backdrop's background means a full-viewport repaint on every frame of the
       focus transition, competing with the box move and the sibling fade. The
       0.2 scrim still appears - it just does not animate. */
    &::backdrop {
      background: rgba(0, 0, 0, 0);
    }

    &:popover-open::backdrop {
      background: rgba(0, 0, 0, 0.2);
    }
  }

  &.expanded .search-container {
    max-width: 540px;
  }

  .input-container {
    anchor-name: --search-input-container;

    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px;
    border-radius: 12px;
    background: ${color.container.background};
    /* One shadow, always the same one: the hover used to raise the
       elevation and the box wore two shades - the owner took the hover
       one out. */
    box-shadow: ${effect.elevation['03']};
    color: ${color.text.primary};
  }

  input {
    flex-grow: 1;
    order: 2;
    border: none;
    padding: 0;
    font: inherit;
    outline: none;
    background: inherit;
  }

  .search-button {
    --leo-icon-size: 24px;

    order: 3;
    padding: 4px;
    border-radius: 4px;
    visibility: hidden;
    opacity: 0;
    color: ${color.icon.secondary};

    transition: opacity var(--self-transition-duration);

    &:hover {
      background-color: ${color.container.interactive};
    }
  }

  &.expanded .search-button {
    visibility: visible;
    opacity: 1;
  }

  .results-container {
    position: fixed;
    position-anchor: --search-input-container;
    position-area: bottom center;

    width: anchor-size(width);
    margin: 12px 0;
    display: flex;
    flex-direction: column;
    visibility: hidden;
    opacity: 0;

    border-radius: 16px;
    background: ${color.container.background};
    overflow: clip;
    box-shadow: ${effect.elevation['01']};

    transition: opacity var(--self-transition-duration);
  }

  &.expanded .results-container {
    visibility: visible;
    opacity: 1;
  }

`
