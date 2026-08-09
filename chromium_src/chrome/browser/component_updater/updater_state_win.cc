/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

namespace component_updater {
namespace {

// growser (#61): our own update namespace, not Brave's.
//
// The component updater reads these to learn what an administrator configured
// for updates. Brave's keys are set by their own updater (Omaha 3), which we do
// not ship, so today the read simply finds nothing either way - but pointing at
// another product's namespace means that on a machine with Brave installed we
// would obey their administrator's policy. The policy root matches #58
// (SOFTWARE\Policies\Growser) and the app GUID matches our install mode.
//
// The IS_BRAVE_ORIGIN_BRANDED split above this in brave-core chose between two
// of their GUIDs; we build neither variant, so one constant is enough.
const wchar_t kGoogleUpdatePoliciesKey[] =
    L"SOFTWARE\\Policies\\Growser\\Update";
const wchar_t kCheckPeriodOverrideMinutes[] = L"AutoUpdateCheckPeriodMinutes";
const wchar_t kUpdatePolicyValue[] = L"UpdateDefault";
const wchar_t kChromeUpdatePolicyOverride[] =
    L"Update{B003E671-954C-4C60-A0D4-4172D74FD4C1}";

// Don't allow update periods longer than six weeks (Chrome release cadence).
const int kCheckPeriodOverrideMinutesMax = 60 * 24 * 7 * 6;

// growser (#61): the updater's own registry state, ours as well.
const wchar_t kRegPathGoogleUpdate[] = L"Software\\Growser\\Update";
const wchar_t kRegPathClientsGoogleUpdate[] =
    L"Software\\Growser\\Update\\Clients\\"
    L"{B003E671-954C-4C60-A0D4-4172D74FD4C1}";

}  // namespace
}  // namespace component_updater

#include <chrome/browser/component_updater/updater_state_win.cc>
