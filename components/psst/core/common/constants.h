// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_PSST_CORE_COMMON_CONSTANTS_H_

namespace psst {

inline constexpr char kBravePsst[] = "bravePsst";

inline constexpr char kBravePsstHost[] = "psst";
inline constexpr char kBraveUIPsstURL[] = "chrome://psst/";

// Growser-230: the report goes to our backend, so "Learn more" goes to our
// page about what the settings do, not to Brave's help centre.
inline constexpr char16_t kPsstReportDialogLearnMoreUrl[] =
    u"https://growser.org/features.html";

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_COMMON_CONSTANTS_H_
