/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/update_util.h"

#include "base/command_line.h"
#include "brave/components/constants/brave_switches.h"
#include "content/public/common/content_switches.h"

namespace brave {

bool UpdateEnabled() {
  // growser (#35): enable Sparkle auto-update in our non-official builds.
  // Upstream gates UpdateEnabled() on OFFICIAL_BUILD; our fork ships
  // non-official Release (the isOfficialBuild() patch in config.ts sets
  // is_official_build=false under GROWSER_NON_OFFICIAL=1), so without this
  // SparkleGlue is never created and auto-update is dead regardless of the
  // feed URL, the EdDSA key and SUPublicEDKey. Consent is implied for our own
  // product (same rationale as crash consent in #33). The two switches are
  // honored exactly as in the official branch: an explicit --disable-brave-
  // update, and browser tests (kTestType) which must not check for updates.
  const base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();
  return !cmdline->HasSwitch(switches::kDisableBraveUpdate) &&
         // Don't check for updates in browser tests.
         !cmdline->HasSwitch(switches::kTestType);
}

}  // namespace brave
