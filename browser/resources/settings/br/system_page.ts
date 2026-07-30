// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// settings-system-page is now a Lit element upstream, so the
// RegisterPolymerTemplateModifications/RegisterPolymerPrototypeModification
// overrides that used to live here no longer apply (they only affect
// Polymer elements). The Shortcuts row and trailing tab/window/fullscreen
// toggles they used to add are now added via a lit_mangler override; see
// chromium_src/chrome/browser/resources/settings/system_page/
// system_page.html.ts.lit_mangler.ts and the companion system_page.ts
// override in the same directory.
export {}
