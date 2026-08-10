/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser (#65): the Start tile manifest has to be named after the executable.
//
// Windows looks for `<exe base name>.VisualElementsManifest.xml` beside the
// exe, and upstream hardcodes "chrome.VisualElementsManifest.xml" here. Our
// executable is growser.exe, so the file we install is never read: the tile
// falls back to the exe icon on a default background, which is what #65 saw in
// the Start menu.
//
// Brave has the same bug for brave.exe and does not override this file - so
// this is worth sending upstream (#52) rather than only keeping. The upstream
// shape would derive the name from install_static rather than hardcode either
// product's, since the constant is otherwise a per-brand copy waiting to be
// forgotten.
#define kVisualElementsManifest kVisualElementsManifest_ChromiumImpl

#include <chrome/installer/setup/setup_constants.cc>

#undef kVisualElementsManifest

namespace installer {

// Declared again on purpose: the #define above renamed the declaration in
// setup_constants.h as the header went through, so without this the definition
// below has internal linkage and the compiler rightly calls it unused.
extern const wchar_t kVisualElementsManifest[];

const wchar_t kVisualElementsManifest[] = L"growser.VisualElementsManifest.xml";

}  // namespace installer
