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

// A drawing is not a favicon: it keeps whatever shape its brand drew it in,
// and a wordmark three times as wide as it is tall, fitted into a square
// slot, comes out a third the height it could be - Adobe, achmea and aruba
// were unreadable for that reason alone. So the slot a PACK drawing gets is
// as wide as two thirds of the tile, and the drawing is fitted into it
// proportionally about its centre. A square mark is unaffected: it still
// meets the height first. The two thirds is what keeps a margin at the
// edges, whatever the shape.
const artShareOfWidth = 0.66

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
    --self-tile-art-width:
      calc(var(--self-tile-width) * ${artShareOfWidth});
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

  /* An icon is only ever scaled DOWN, and the limit is counted in the screen's
     own pixels rather than CSS ones. At 150% scaling a 56px slot is 84 real
     pixels, so a 64px icon stretched to fill it is still stretched - which is
     how apple.com stayed soft after the first attempt at this. Capped by its
     own resolution, it draws smaller and sharp. */
  .top-site-icon {
    width: auto;
    height: auto;
    max-width:
      min(
        var(--self-tile-icon-size),
        calc(var(--self-icon-natural, 9999) * 1px / var(--self-dpr, 1)));
    max-height:
      min(
        var(--self-tile-icon-size),
        calc(var(--self-icon-natural, 9999) * 1px / var(--self-dpr, 1)));
    flex: 0 0 auto;
    pointer-events: none;
  }

  /* A drawing from the pack, carried as a data URL rather than inlined: the
     page enforces Trusted Types and innerHTML is not available to it. A
     single-colour mark has its white written in before it is handed over,
     because an image cannot inherit a colour from the page around it. */
  .top-site-art {
    display: flex;
    align-items: center;
    justify-content: center;
    width: var(--self-tile-art-width);
    height: var(--self-tile-icon-size);
    flex: 0 0 auto;
    pointer-events: none;
  }

  .top-site-art {
    object-fit: contain;
  }

  /* A brand the pack knows but has no drawing for. The name is the tile, so
     there is nothing to reveal on hover and no caption underneath. */
  .top-site-name {
    color: ${color.white};
    font: ${font.default.semibold};
    text-align: center;
    padding: 0 12px;
    line-height: 1.2;
    max-height: 100%;
    overflow: hidden;
    text-shadow: 0 1px 4px rgba(0, 0, 0, 0.35);
    pointer-events: none;
  }

  .top-site-monogram {
    color: ${color.white};
    font-size: calc(var(--self-tile-icon-size) * 0.72);
    font-weight: 600;
    line-height: 1;
    text-shadow: 0 1px 4px rgba(0, 0, 0, 0.35);
    pointer-events: none;
  }

  .add-tile {
    --leo-icon-size: 32px;
    --leo-icon-color: ${color.white};
    --self-tile-color: rgba(255, 255, 255, 0.22);

    backdrop-filter: blur(30px);
    box-shadow: none;
  }

  /* The caption is white, and a tile is now allowed to be nearly white when
     that is what makes its logo readable - so the caption gets its own
     ground rather than relying on the tile being dark. It fades in with the
     label, so a resting tile is still only its colour and its mark. The
     owner's rule for its shape: the shade stands only as tall as the text
     and its paddings - it must not curtain the drawing above. */
  .top-site-tile::after {
    content: '';
    position: absolute;
    left: 0;
    right: 0;
    bottom: 0;
    height: 34px;
    border-radius: inherit;
    background: linear-gradient(
      to bottom, rgba(0, 0, 0, 0), rgba(0, 0, 0, 0.62));
    pointer-events: none;
  }

  .top-site-label {
    position: absolute;
    left: 8px;
    right: 8px;
    bottom: 8px;
    /* Always the page's address, always VISIBLE, always at full contrast:
       pure white ABOVE the shade - the shade is a ::after sibling that
       paints after children, and without the z-index it veiled the text
       into grey. */
    z-index: 1;
    color: #fff;
    font: ${font.small.semibold};
    text-align: center;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    text-shadow: 0 1px 3px rgba(0, 0, 0, 0.85);
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
