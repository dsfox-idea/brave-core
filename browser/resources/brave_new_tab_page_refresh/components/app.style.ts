/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color, effect, font } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const narrowBreakpoint = '900px'
export const threeColumnBreakpoint = '1275px'
export const horizontalContentPadding = 24

const topControlsNarrowBreakpoint = '1075px'

export const style = scoped.css`
  & {
    --search-transition-duration: 120ms;
    --top-controls-text-shadow: rgba(0, 0, 0, 0.33) 0 1px 2px;

    /* growser (#117, #136): how far below its slot the search row is drawn.
     * It lives here, at the page root, rather than in search_box.style.ts,
     * because two things need it now: the row itself, which translates by it,
     * and the clock, which is pinned 100px below the row and must compute
     * that from the row's SLOT - the row itself teleports into the top layer
     * when it expands, and an anchor that disappears drops its anchored
     * element back to the top of the page. Custom properties inherit down, so
     * the search box still reads it from here. */
    --search-expand-travel: 13vh;

    /* As of CR146, scrollbars for elements slotted into shadow DOM trees
     * sometimes do not respect the user's color scheme. Adding a rule for
     * "color-scheme" ensures that scrollbars are consistent.
     */
    @media (prefers-color-scheme: dark) {
      color-scheme: dark;
    }
  }

  @keyframes background-scroll-fade {
    from {
      background: rgba(0, 0, 0, 0);
      backdrop-filter: blur(0);
    }
    50% {
      backdrop-filter: blur(0);
    }
    to {
      background: rgba(0, 0, 0, 0.65);
      backdrop-filter: blur(32px);
    }
  }

  .background-filter {
    position: fixed;
    inset: 0;
    z-index: 1;

    animation: linear background-scroll-fade both;
    animation-timeline: scroll();
    animation-range: 0px 100vh;
  }

  .settings {
    --leo-icon-size: 18px;

    position: absolute;
    z-index: 2;
    inset-block-start: 0;
    inset-inline-end: 0;
    margin: 24px;

    padding: 8px;
    border-radius: 50%;
    color: #fff;
    opacity: .9;
    filter: drop-shadow(var(--top-controls-text-shadow));

    &:hover {
      background: rgba(255, 255, 255, .3);
      box-shadow: ${effect.elevation['01']};
      cursor: pointer;
    }

    @container (width < ${topControlsNarrowBreakpoint}) {
      margin: 12px;
    }
  }

  /* growser: the clock sits at the bottom of the screen, centred, and carries
   * the date under the time. Upstream tucks it into the top-left corner at
   * 56px, where it reads as a label on the wallpaper; centred and larger it
   * becomes the thing the page is for when nothing else is on it. */
  .clock {
    /* growser (#136): pinned 100px below the SEARCH ROW and nailed there -
     * a resize, a narrower window, a taller or shorter search box must not
     * change that distance. Hence CSS anchor positioning rather than the flow
     * it used to sit in: the anchor is the search row itself, so the clock
     * follows it instead of following whatever else the column contains.
     * (The caption below uses the same mechanism against the widget
     * container.) Centred by spanning the anchor's inline extent and letting
     * the margins do the work: the anchor-center keyword is newer than the
     * Chromium this builds against. Note there are no backticks in this
     * comment - the whole stylesheet is a template literal, and one would end
     * it. */
    position: absolute;
    /* Anchored to the search row's SLOT, offset by the same travel the row
     * itself uses - so the distance a person sees is 100px from the bottom of
     * the row, computed from something that never moves.
     *
     * Two wrong ways were tried first, and both are worth remembering.
     * Anchoring to the container WITHOUT the travel measured a tidy 100px and
     * put the clock 12px on top of the search box. Anchoring to the visible
     * row fixed that and introduced a worse fault: the row becomes a popover
     * on focus and leaves the flow, its anchor stops resolving, and the clock
     * drops to its static position - the top of the page, over the tiles -
     * which is the flash the owner saw. */
    position-anchor: --ntp-search-row;
    inset-block-start: calc(
      anchor(end) + var(--search-expand-travel) + 100px);
    inset-inline-start: anchor(start);
    inset-inline-end: anchor(end);
    margin-inline: auto;

    z-index: 2;
    width: fit-content;

    padding: 8px 16px;
    font: ${font.large.semibold};
    font-size: 96px;
    font-weight: 500;
    line-height: 100%;
    /* the wallpaper can be bright, and white on a pale sky is unreadable with
     * the 1px shadow the top controls use - this one carries the text over
     * anything */
    text-shadow: 0 1px 2px rgba(0, 0, 0, .40), 0 2px 24px rgba(0, 0, 0, .35);

    color: #fff;
    opacity: .9;

    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;

    .time {
      display: flex;
      align-items: flex-start;
      gap: 0;
    }

    .date {
      font-size: 20px;
      font-weight: 400;
      line-height: 120%;
      letter-spacing: .01em;
      opacity: .9;

      /* most locales write the weekday and the month in lower case, and
       * text-transform: capitalize raises every word - which in Russian turns
       * "22 august 2026 y." into "22 August 2026 Y." Only the first letter of
       * the line goes up. */
      &::first-letter {
        text-transform: uppercase;
      }
    }


    .day-period {
      font-size: 14px;
      font-weight: 700;
      line-height: 14px;
      margin-top: 7px;
    }

    /* growser: upstream shrinks the clock to 16px and hides the AM/PM marker
     * once the window narrows, because there it shares the top row with the
     * top-site tiles and the settings button. Ours lives at the bottom of the
     * page on its own, so there is nothing to make room for - it keeps its size
     * at every width. */
  }

  main {
    container-type: inline-size;
    view-timeline-name: --ntp-main-view-timeline;
    position: relative;
    z-index: 2;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    gap: 16px;
    padding: 16px ${horizontalContentPadding}px;

    /* growser (#117): siblings fade out on search focus with opacity only.
       Upstream also scales them to 0.9, which repaints six subtrees (top-site
       tiles, live widgets, clock) on every frame at the same time as the box
       moves and the backdrop dims. Opacity alone reads the same "recede" and
       stays cheap. */
    > * {
      transition:
        opacity var(--search-transition-duration),
        visibility var(--search-transition-duration) allow-discrete;

      .search-box-expanded & {
        opacity: 0;
        visibility: hidden;
      }
    }
  }

  .topsites-container {
    padding: 16px 0;
    align-self: stretch;
    display: flex;
    gap: 16px;
  }

  .searchbox-container {
    align-self: stretch;

    /* What the clock hangs from: this stays in the flow whatever the search
     * row does. */
    anchor-name: --ntp-search-row;

    .search-box-expanded & {
      opacity: 1;
      visibility: visible;
    }
  }

  .spacer {
    flex: 1 1 auto;
    align-self: stretch;

    display: flex;
    align-items: stretch;
    justify-content: center;

    @container (width > ${narrowBreakpoint}) {
      min-height: 200px;
    }
  }

  .caption-container {
    @container (width > ${narrowBreakpoint}) {
      position: absolute;
      position-anchor: --ntp-widget-container;
      inset-block-end: anchor(end);
      inset-inline-end: anchor(start);
      inset-inline-start: 0;
      margin-inline-start: 16px;
      margin-inline-end: 16px;

      min-width: fit-content;
      min-height: 30px;

      position-try-fallbacks: --try-captions-above;
    }
  }

  @position-try --try-captions-above {
    inset-block-end: anchor(start);
    inset-inline-start: anchor(start);
    inset-inline-end: unset;
    margin-block-end: 16px;
    margin-inline-start: 0;
    margin-inline-end: 0;
  }

  .caption-container:has(.centered-ntt-cta-button) {
    position: static;
  }

  .widget-container {
    --widget-height: 128px;
    --widget-min-width: 380px;
    --widget-max-width: 512px;
    --widget-gap: 16px;

    anchor-name: --ntp-widget-container;

    flex: 0 0 auto;
    min-height: var(--widget-height);

    display: grid;
    grid-auto-columns: minmax(var(--widget-min-width), var(--widget-max-width));
    grid-auto-rows: minmax(var(--widget-height), auto);
    grid-auto-flow: column;
    justify-content: center;
    align-items: stretch;
    gap: var(--widget-gap);

    &:empty {
      min-height: 0;
    }

    @container (width <= ${narrowBreakpoint}) {
      grid-auto-flow: row;
    }

    @container (width > ${narrowBreakpoint}) {
      &:has(> :nth-child(2)) {
        justify-content: space-between;
        align-self: stretch;
      }

      &:has(> :nth-child(3)) {
        justify-content: center;
        align-self: center;
      }
    }
  }

  .news-container {
    position: relative;
    z-index: 1;
  }
`

style.passthrough.css`
  .allow-background-pointer-events {
    /* This element will allow pointer events to target the background. */
    pointer-events: none;

    /* But children will not (unless they explicitly allow it). */
    > :not(.allow-background-pointer-events) {
      pointer-events: auto;
    }

    /* And not when a popover is open. When a popover is open and the background
       contains an interactive iframe, pointer events on a background iframe
       will not "light-dismiss" the popover. */
    :scope:has(:popover-open) & {
      pointer-events: auto;
    }
  }

  & {
    font: ${font.default.regular};
    color: ${color.text.primary};
    interpolate-size: allow-keywords;
  }

  button {
    margin: 0;
    padding: 0;
    background: 0;
    border: none;
    text-align: unset;
    width: unset;
    font: inherit;
    cursor: pointer;

    &:disabled {
      cursor: default;
    }
  }

  h2 {
    font: ${font.heading.h2};
    margin: 0;
  }

  h3 {
    font: ${font.heading.h3};
    margin: 0;
  }

  h4 {
    font: ${font.heading.h4};
    margin: 0;
  }

  p {
    margin: 0;
  }

  dialog, [popover] {
    border: none;
    color: inherit;
    margin: 0;
    padding: 0;
    background: none;

    &::backdrop {
      background-color: transparent;
    }
  }

  .popover-menu {
    padding: 4px;
    border-radius: 8px;
    border: solid 1px ${color.divider.subtle};
    background: ${color.container.background};
    box-shadow: 0 1px 0 0 rgba(0, 0, 0, 0.05);
    display: flex;
    flex-direction: column;
    gap: 4px;
    min-width: 180px;

    .divider {
      height: 1px;
      background: ${color.divider.subtle};
    }

    button, a {
      --leo-icon-size: 20px;
      padding: 8px 24px 8px 8px;
      border-radius: 4px;
      display: flex;
      align-items: center;
      gap: 16px;
      color: inherit;
      text-decoration: none;

      &:hover, &.highlight {
        background: ${color.container.highlight};
      }
    }
  }

  .skeleton {
    --self-animation-color: rgba(0, 0, 0, 0.1);

    background: rgba(255, 255, 255, 0.25);
    position: relative;
    overflow: hidden;
    opacity: .7;

    animation: skeleton-fade-in 1s ease-in-out both 250ms;

    @media (prefers-color-scheme: dark) {
      --self-animation-color: rgba(255, 255, 255, 0.1);
    }
  }

  .skeleton:after {
    content: '';
    position: absolute;
    transform: translateX(-100%);
    inset: 0;
    background: linear-gradient(
      90deg, transparent, var(--self-animation-color), transparent);
    animation: skeleton-background-cycle 2s linear 0.5s infinite;
  }

  @keyframes skeleton-fade-in {
    0% { opacity: 0; }
    100% { opacity: .7; }
  }

  @keyframes skeleton-background-cycle {
    0% { transform: translateX(-100%); }
    50% { transform: translateX(100%); }
    100% { transform: translateX(100%); }
  }
`
