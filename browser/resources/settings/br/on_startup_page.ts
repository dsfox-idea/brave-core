// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { injectStyle } from '//resources/brave/lit_overriding.js'
import { css } from '//resources/lit/v3_0/lit.rollup.js'
import type {
  CrLitElement,
  PropertyValues
} from '//resources/lit/v3_0/lit.rollup.js'

import {
  SettingsOnStartupPageElement
} from '../on_startup_page/on_startup_page.js'

// `expanded` (see br/settings_section.ts) suppresses that file's
// `!important` Leo card look, but the plain upstream #card rule underneath
// still draws a shadow/background/radius from these CSS vars. Custom
// properties inherit across shadow boundaries, so setting them here reaches
// settings-section's #card without needing to touch its shadow root.
injectStyle(SettingsOnStartupPageElement, css`
  settings-section {
    --cr-card-background-color: transparent;
    --cr-card-border-radius: 0;
    --cr-card-shadow: none;
  }
`)

const modifyOnStartupPage = async (root: ShadowRoot) => {
  // We want this section's controls but not its own header or card look.
  // Unlike default_browser_page.ts, we can't unwrap settings-section by
  // moving its children up a level: that disconnects/reconnects them,
  // permanently breaking settings-radio-group/controlled-radio-button's
  // one-time pref-key mirroring. So leave the children in place, opt out of
  // the card styling via the `expanded` class, and just remove the header.
  const settingsSection = root.querySelector('settings-section') as
      CrLitElement | null
  if (!settingsSection) {
    throw new Error('[Settings] Missing settings-section on on_startup_page')
  }
  settingsSection.classList.add('expanded')

  // `settings-section` is itself a Lit element that hasn't rendered yet at
  // this point (its first render is scheduled as a microtask, which runs
  // after our own `firstUpdated`), so wait for it before touching its shadow
  // root.
  await settingsSection.updateComplete

  settingsSection.shadowRoot?.getElementById('header')?.remove()
}

// `firstUpdated` is `protected` on ReactiveElement, so reach it through an
// untyped view of the prototype to patch it from outside the class hierarchy.
const proto = SettingsOnStartupPageElement.prototype as unknown as {
  firstUpdated?: (changedProperties: PropertyValues) => void
}
const originalFirstUpdated = proto.firstUpdated
proto.firstUpdated = function(
    this: SettingsOnStartupPageElement,
    changedProperties: PropertyValues) {
  originalFirstUpdated?.call(this, changedProperties)
  if (this.shadowRoot) {
    modifyOnStartupPage(this.shadowRoot)
  }
}
