/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_INSTALLER_UTIL_BRAVE_SHELL_UTIL_H_
#define BRAVE_INSTALLER_UTIL_BRAVE_SHELL_UTIL_H_

#include <string>

namespace installer {

std::wstring GetProgIdForFileType();
bool ShouldUseFileTypeProgId(std::wstring_view ext);

// Growser-365: the AUMID of the MSIX package this process runs from, empty when
// it is not packaged. Inside a package Windows honours no other id: a window,
// jump list or pin under any other id is a second app on the taskbar.
std::wstring GetPackageAppUserModelId();

}  // namespace installer

#endif  // BRAVE_INSTALLER_UTIL_BRAVE_SHELL_UTIL_H_
