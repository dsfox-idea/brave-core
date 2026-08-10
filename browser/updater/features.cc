/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/features.h"

#include <optional>

#include "base/logging.h"
#include "base/time/time.h"

namespace brave_updater {

// growser (#51): enabled. Brave keeps it off while they migrate from their
// Omaha 3, which we never had - our updater is Omaha 4 and our update endpoint
// answers only that protocol.
BASE_FEATURE(kBraveUseOmaha4, base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE_PARAM(int,
                   kLegacyFallbackIntervalDays,
                   &kBraveUseOmaha4,
                   "legacy-fallback-interval-days",
                   5);

namespace {

// We cache the result of ShouldUseOmaha4() to ensure that it stays constant
// across multiple calls.
std::optional<bool> g_use_omaha4;

bool ShouldUseOmaha4Impl(base::Time now, std::optional<bool>& state) {
  if (!state.has_value()) {
    // Whether Omaha 4 should be used is mostly determined by the feature flag.
    // However, we also want to give the legacy implementation a chance to run
    // every X days. This lets us recover from a situation where updates with
    // Omaha 4 are broken because of a bug. Once Omaha 4 is stable, we can
    // remove the periodic fallback.
    // growser (#51): no periodic fallback. Upstream lets the LEGACY updater run
    // every few days so that a bug in Omaha 4 cannot strand users - a sound
    // idea when you have two working updaters. We have one: there is no Omaha 3
    // in this build and no server for it, so falling back would simply mean no
    // update check that day, and a user unlucky with the calendar could sit on
    // an old build indefinitely without anything looking wrong.
    state = base::FeatureList::IsEnabled(kBraveUseOmaha4);
    VLOG(1) << "Using Omaha 4: " << state.value();
  }
  return state.value();
}

}  // namespace

bool ShouldUseOmaha4() {
  return ShouldUseOmaha4Impl(base::Time::Now(), g_use_omaha4);
}

bool ShouldUseOmaha4ForTesting(base::Time now, std::optional<bool>& state) {
  return ShouldUseOmaha4Impl(now, state);
}

}  // namespace brave_updater
