/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"

#include <third_party/blink/common/features_generated.cc>

namespace blink::features {
OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kAIProofreadingAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIPromptAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIPromptAPIMultimodalInput, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIRewriterAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAISummarizationAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kAIWriterAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    {kLanguageDetectionAPI, base::FEATURE_DISABLED_BY_DEFAULT},
    // growser (#70): kTranslationAPI stays ENABLED - it is what gives us page
    // translation at all. Brave turns it off because the API is a fingerprinting
    // surface (Translator.availability() tells a site which language packs are
    // installed, which differs per user) and because it runs a Google binary
    // locally. Weighed against that: without it translation does not work in
    // this fork at all - Brave's endpoint refuses a fork, Google's wants a key
    // Chrome has and we do not, and a paid provider was declined (#45). Chrome
    // exposes this API to every site by default, so the exposure is the
    // ordinary one rather than something unusual to us.
    //
    // The neighbours above stay disabled: they are the writing and prompting
    // APIs, which buy us nothing.
    {kUserMediaElement, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace blink::features
