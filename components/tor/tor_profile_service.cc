/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_profile_service.h"

#include "base/time/time.h"
#include "brave/components/tor/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"

namespace tor {

TorProfileService::TorProfileService() = default;

TorProfileService::~TorProfileService() = default;

// static
void TorProfileService::RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // growser (#78): Tor is off by default. The Tor client itself arrives as a
  // component, and our component updater gets 403 from Brave for want of a
  // service key, so Tor cannot run in this browser at all. Disabling the
  // commands and hiding the settings section was not enough - "open link in
  // Tor window" survived in the page context menu, which is gated on this
  // pref rather than on the commands. This is Brave's own switch (the
  // TorDisabled policy sets the same pref), so every surface that asks
  // IsTorDisabled() is closed at once, including any we did not enumerate.
  // Growser-157: Tor is on again where a client can actually arrive. growser#78
  // turned it off because the client is a component and Brave's server answers
  // a fork 403; we publish our own now (scripts/make-tor-component.py,
  // deploy/growser-backend), but only for Windows. On the other platforms no
  // package exists yet, and offering Tor there would be the same promise this
  // pref was set to stop making.
#if BUILDFLAG(IS_WIN)
  registry->RegisterBooleanPref(prefs::kTorDisabled, false);
#else
  registry->RegisterBooleanPref(prefs::kTorDisabled, true);
#endif
  registry->RegisterDictionaryPref(prefs::kBridgesConfig);
  registry->RegisterTimePref(prefs::kBuiltinBridgesRequestTime, base::Time());
}

// static
void TorProfileService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kAutoOnionRedirect, false);
  registry->RegisterBooleanPref(prefs::kOnionOnlyInTorWindows, true);
}

}  // namespace tor
