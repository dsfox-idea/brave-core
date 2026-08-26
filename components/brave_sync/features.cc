/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/features.h"

#include "base/feature_list.h"

namespace brave_sync::features {

// growser (#78/#43): sync is off until we run a sync server of our own.
//
// This feature, not the command line, is the switch that matters.
// ChromeBrowserMainParts::PreProfileInit appends --disable-sync when it is off
// and calls RemoveSwitch(kDisableSync) when it is on - so it deletes the switch
// anyone else appended, including ours. An earlier attempt appended the switch
// from BraveMainDelegate::AppendCommandLineOptions, which runs first and was
// therefore undone a moment later; the settings tree still offered Sync, and
// the unit test guarding it passed the whole time, because it asserted that
// AppendCommandLineOptions appends - not that the switch survives to where it
// is read.
//
// Turning the feature off makes Brave's own code append the switch, and
// everything downstream follows: IsSyncAllowedByFlag(), the "isSyncDisabled"
// value the settings page reads, the /braveSync route, and the menu command.
BASE_FEATURE(kBraveSync, base::FEATURE_DISABLED_BY_DEFAULT);
BASE_FEATURE(kBraveSyncDefaultPasswords,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace brave_sync::features
