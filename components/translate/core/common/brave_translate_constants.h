/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_CONSTANTS_H_
#define BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_CONSTANTS_H_

namespace translate {

// growser (#45): our backend, not Brave's. Translate sends the text of every
// page a user asks to translate. Brave's endpoint is a proxy in front of Google
// - it hides the user's IP from Google and shows it to Brave instead. Ours does
// the same job with us as the intermediary, and keeps the Google API key out of
// the binary the way #41 and #42 already do.
//
// It could not have worked as it was, either: the request carries
// BUILDFLAG(BRAVE_SERVICES_KEY), which a fork does not have (lesson 30).
//
// These paths are a contract with brave_translate.js: it rewrites
// /translate_static/ to kBraveTranslateStaticPath and points every request at
// this origin's host, so /translate_a/t lands at the origin root.
inline constexpr char kBraveTranslateOrigin[] = "https://backend.growser.org";
inline constexpr char kBraveTranslateScriptURL[] =
    "https://backend.growser.org/translate/static/v1/element.js";

// The used version of translate static resources (js/css files).
// Used in brave_translate.js as a replacement to /translate_static/ part in
// original script URLs.
inline constexpr char kBraveTranslateStaticPath[] = "/translate/static/v1/";

}  // namespace translate

#endif  // BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_CONSTANTS_H_
