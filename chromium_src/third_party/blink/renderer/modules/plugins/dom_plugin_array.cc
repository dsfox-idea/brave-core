/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/plugins/dom_plugin_array.h"

// growser (#82): navigator.plugins is left exactly as Chrome reports it.
//
// Brave farbled this list: it renamed the built-in PDF plugins to generated
// names ("Chromium portable-document-format plug-in" and the like), appended
// two invented plugins, and shuffled the order. The result was a list of seven
// with per-origin names.
//
// Farbling is the right instinct for a surface that differs between users -
// canvas, audio, WebGL - because there the noise hides you in a crowd. Plugins
// are the opposite: every Chrome on every machine reports the same five, with
// the same names, in the same order. Adding noise there does not hide anyone,
// it marks them. A site that sees seven plugins with names no Chrome has ever
// produced learns more about the visitor than the real list would ever have
// told it, and it learns it in one property read.
//
// So this override now does nothing but exist, because the upstream file is
// still included through it and the empty define keeps the patch point.
#define BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__FARBLE_PLUGIN_DATA

#include <third_party/blink/renderer/modules/plugins/dom_plugin_array.cc>

#undef BRAVE_DOM_PLUGINS_UPDATE_PLUGIN_DATA__FARBLE_PLUGIN_DATA
