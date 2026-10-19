/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/installer/util/brave_shell_util.h"

#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// growser#365: the package id replaces the installer's only inside an MSIX
// package. A test process has no package identity, so here every id must stay
// what the installer-based build has always used; the packaged side is
// measured on a registered package (docs/windows.md).
TEST(GrowserPackageAppUserModelIdTest, EmptyOutsideAPackage) {
  EXPECT_EQ(installer::GetPackageAppUserModelId(), std::wstring());
}

TEST(GrowserPackageAppUserModelIdTest, BrowserModelIdUnchangedOutsideAPackage) {
  EXPECT_EQ(ShellUtil::GetBrowserModelId(/*is_per_user_install=*/false),
            std::wstring(install_static::GetBaseAppId()));
}
