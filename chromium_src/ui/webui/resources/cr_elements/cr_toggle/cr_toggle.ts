// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '//resources/brave/leo.bundle.js'

import {CrLitElement, css} from '//resources/lit/v3_0/lit.rollup.js';
import type {CSSResultGroup} from '//resources/lit/v3_0/lit.rollup.js';

import {getHtml} from './cr_toggle.html.js';

export const MOVE_THRESHOLD_PX: number = 5;

export class CrToggleElement extends CrLitElement {
  static get is() {
    return 'cr-toggle';
  }

  declare $: {
    toggle: HTMLElement
    knob: HTMLElement
  }

  // growser (#139): the toggle gets 4px corners of its own.
  //
  // The product ceiling is 12px, and it leaves this control exactly as round
  // as it was: a border-radius is clamped to half the box, the bar is 27px
  // tall, so anything at or above 13.7px draws the same pill. The value that
  // shows a corner has to be smaller than that, which is a decision about
  // this control rather than about the scale.
  //
  // Set on the host: custom properties inherit through the shadow boundary,
  // and the bar lives in a closed shadow root that cannot be reached any
  // other way. The thumb reads the same property and squares off with it -
  // chosen by the owner from 12 / 8 / 6 / 4 rendered on the real page.
  static override get styles(): CSSResultGroup {
    return css`
      :host {
        --leo-radius-full: 4px;
      }
    `
  }

  override render() {
    return getHtml.bind(this)();
  }

  static override get properties() {
    return {
      checked: {
        type: Boolean,
        reflect: true,
        notify: true,
      },

      disabled: {
        type: Boolean,
        reflect: true,
      },
    };
  }

  accessor checked: boolean = false;
  accessor disabled: boolean = false;

  override firstUpdated(){
    this.addEventListener('click', e => {
      // Prevent |click| event from bubbling. It can cause parents of this
      // elements to erroneously re-toggle this control.
      e.stopPropagation();
      e.preventDefault();
    })
  }

  // The Nala event looks a bit different to the Chromium one, so we need to
  // convert it.
  async onChange_(e: { checked: boolean }) {
    this.checked = e.checked

    // Yield, so that 'checked-changed' (originating from `notify: 'true'`) fire
    // before the 'change' event below, which guarantees that any Polymer parent
    // with 2-way bindings on the `checked` attribute are updated first.
    await this.updateComplete

    this.fire('change', this.checked)
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'cr-toggle': CrToggleElement;
  }
}

customElements.define(CrToggleElement.is, CrToggleElement);
