/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/on_device_translation/features.h"

// growser (#70): pre-install the translation model instead of waiting for a
// page to ask for it.
//
// The Translator API refuses to download anything unless the page has a user
// gesture, and our translation starts from browser UI - a click there gives the
// page no gesture at all. Measured: NotAllowedError, with availability stuck at
// "downloadable" forever. So nothing can ever bootstrap it from the page side,
// and the browser has to do it.
//
// The cost is honest and worth stating: every user downloads the pack whether
// they translate or not. That is the v1 trade - the alternative is starting the
// download on the first translate request and making the user wait through it,
// which needs UI of its own to not look like a hang. Planned separately, along
// with more than one language pair.
//
// "en-ru" is one pack, not two: every TranslateKit pack is anchored on English
// (see kLanguagePackComponentConfigMap), so this is the pair our Russian UI
// needs and the pivot for anything added later.
#define kAutoDownloadTranslateLanguagePacksLanguagePairs \
  kAutoDownloadTranslateLanguagePacksLanguagePairs_ChromiumImpl

#include <components/on_device_translation/features.cc>

#undef kAutoDownloadTranslateLanguagePacksLanguagePairs

#include "base/feature_override.h"

namespace on_device_translation {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kAutoDownloadTranslateLanguagePacks, base::FEATURE_ENABLED_BY_DEFAULT},
}});

const base::FeatureParam<std::string>
    kAutoDownloadTranslateLanguagePacksLanguagePairs{
        &kAutoDownloadTranslateLanguagePacks, "language_pairs", "en-ru"};

}  // namespace on_device_translation
