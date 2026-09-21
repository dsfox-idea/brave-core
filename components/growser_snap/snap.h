/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_GROWSER_SNAP_SNAP_H_
#define BRAVE_COMPONENTS_GROWSER_SNAP_SNAP_H_

#include <string_view>

namespace base {
class Environment;
}

namespace growser {

// The snap (src/brave/snapcraft.yaml) sets GROWSER_SNAP=1 for the app;
// nothing else does. This is the one place the variable's name lives.
bool IsRunningInSnap(base::Environment* env);

// Growser-256: the snap starts the browser with --no-sandbox, because the
// store refused the browser-support grant that either of Chromium's Linux
// sandboxes needs (growser#226). Chromium warns about that switch on every
// start as if a person had typed it. Inside the snap it is a packaging
// decision the user did not make and cannot change, so the warning about it
// is suppressed there - and only that one: every other bad flag still warns.
// `flag` is a switch name without the leading dashes, as in kBadFlags.
bool IsSnapPackagingFlag(std::string_view flag);

}  // namespace growser

#endif  // BRAVE_COMPONENTS_GROWSER_SNAP_SNAP_H_
