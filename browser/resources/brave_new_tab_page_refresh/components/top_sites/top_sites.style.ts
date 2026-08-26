/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

// The tile at its largest. Below a window wide enough for eight of them the
// tiles shrink instead of the row wrapping, because how many tiles sit in a
// row is a rule the owner set, not a consequence of the window.
export const tileWidth = 168

// Width : height, and the icon as a share of the height. Everything else is
// derived, so one number changes the size of the board.
const tileAspect = 1.5
const iconShareOfHeight = 0.5

// Eight tiles, seven gaps, and the room the menu button and its opposite
// spacer take either side.
const gap = 16
const asideWidth = 24

// What a tile is painted when its favicon has not been read yet, or has no
// colour to give. Neutral rather than branded: a wrong guess that looks like a
// deliberate choice is worse than one that looks like a placeholder.
export const defaultTileColor = '#4a5058'

// The brand gradient runs #33C57B to #148C50 - one hue, lightness 0.49 down to
// 0.31. A tile reproduces that fall around whatever colour its favicon gave,
// so every tile is lit the same way the logo is.
const gradientTopMix = '16%'
const gradientBottomMix = '18%'

export const style = scoped.css`
  & {
    --self-tile-width:
      min(
        ${tileWidth}px,
        (100cqw - ${gap * 7 + asideWidth * 2}px) / 8);
    --self-tile-height: calc(var(--self-tile-width) / ${tileAspect});
    --self-tile-icon-size:
      calc(var(--self-tile-height) * ${iconShareOfHeight});
    --self-transition-duration: 160ms;

    width: 100%;
    container-type: inline-size;
  }

  .top-sites {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    width: fit-content;
    max-width: 100%;
    margin: 0 auto;
  }

  /* Keeps the grid centred against the menu button on the other side. */
  .left-spacer {
    flex: 0 0 ${asideWidth}px;
  }

  .top-site-rows {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: ${gap}px;
  }

  .top-site-row {
    display: flex;
    gap: ${gap}px;
  }

  .top-site-tile {
    position: relative;
    width: var(--self-tile-width);
    height: var(--self-tile-height);
    border-radius: 20px;
    display: flex;
    align-items: center;
    justify-content: center;
    text-decoration: none;
    overflow: hidden;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.18);

    background: linear-gradient(
      180deg,
      color-mix(in oklab, var(--self-tile-color), white ${gradientTopMix}) 0%,
      color-mix(in oklab, var(--self-tile-color), black ${gradientBottomMix})
        100%);

    /* Only the transform animates, so a hover costs the compositor a matrix
       and nothing else - no layout, no paint. */
    transition: transform var(--self-transition-duration) ease-out;
    will-change: transform;

    &:hover, &:focus-visible {
      transform: scale(1.1);
      z-index: 1;
      outline: none;
    }

    &:focus-visible {
      box-shadow: 0 0 0 2px rgba(255, 255, 255, 0.9);
    }
  }

  .top-site-icon {
    width: var(--self-tile-icon-size);
    height: var(--self-tile-icon-size);
    flex: 0 0 auto;
    object-fit: contain;
    pointer-events: none;
  }

  .add-tile {
    --leo-icon-size: 32px;
    --leo-icon-color: ${color.white};
    --self-tile-color: rgba(255, 255, 255, 0.22);

    backdrop-filter: blur(30px);
    box-shadow: none;
  }

  .top-site-label {
    position: absolute;
    left: 8px;
    right: 8px;
    bottom: 10px;
    color: ${color.white};
    font: ${font.small.semibold};
    text-align: center;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    text-shadow: 0 1px 4px rgba(0, 0, 0, 0.45);

    opacity: 0;
    transition: opacity var(--self-transition-duration) ease-out;
  }

  .top-site-tile:hover .top-site-label,
  .top-site-tile:focus-visible .top-site-label {
    opacity: 1;
  }

  .menu-button {
    --leo-icon-color: rgba(255, 255, 255, .8);
    --leo-icon-size: 24px;

    anchor-name: --top-sites-menu-button;

    height: var(--self-tile-height);
    display: flex;
    align-items: center;
    opacity: 0;
    border-radius: 12px;
    visibility: hidden;
    pointer-events: none;

    transition: opacity var(--self-transition-duration);

    &:focus-visible {
      opacity: 1;
      background: rgba(255, 255, 255, .35);
      outline: none;
    }
  }

  .menu-divider {
    border-top: solid 1px ${color.divider.subtle};

    &:first-child, &:last-child {
      display: none;
    }
  }

  .top-sites-menu {
    position-anchor: --top-sites-menu-button;
    position-area: block-end span-inline-start;
    position-try-fallbacks: flip-inline, flip-block;
  }

  .top-site-context-menu-anchor {
    position: absolute;
    top: var(--self-context-menu-y, 0);
    left: var(--self-context-menu-x, 0);
    anchor-name: --top-site-context-menu-anchor;
  }

  .top-site-context-menu {
    position: absolute;
    position-anchor: --top-site-context-menu-anchor;
    position-area: block-end span-inline-end;
    position-try-fallbacks: flip-inline, flip-block;
    margin: -8px;
  }

  &:hover, :has(:popover-open) {
    .menu-button {
      opacity: 1;
      visibility: visible;
      pointer-events: auto;
    }
  }
`
