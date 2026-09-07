/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser (#206): the user data directory migration.
//
// These are written so that the DANGEROUS implementation fails them, not so
// that the written one passes. The tempting one-liner here is base::Move: it
// is shorter, it needs no staging directory and it leaves no second copy on
// the disk - and it would take the directory away from Brave, which owns it
// as much as we do. So the first test asserts what is LEFT BEHIND, not only
// what arrives; a move passes every other assertion in it.
//
// That was measured rather than assumed: with the implementation replaced by
// base::Move, TheLegacyDirectoryIsNotOursToTake and
// AnInterruptedCopyIsNotMistakenForAProfile both fail and the other five pass.
//
// The symlink test does NOT discriminate between implementations, and saying
// so is the point of writing it down. It exists because SingletonLock - a
// symlink to a "hostname-pid" string that is not a path - and SingletonSocket
// are in the directory for as long as a browser is running on it, and a
// migration that trips over them would work on the machine it was written on
// and hand an empty browser to anyone whose Brave happened to be open. The
// first version of this file claimed that base::CopyDirectory fails on them,
// which is why the copy was hand-written; measuring it showed CopyDirectory
// passes this test, because it decides on the enumerator's lstat BEFORE it
// opens anything. The hand-written walk was deleted and this test kept: it
// pins the condition, not the choice.

#include "brave/app/brave_main_delegate.h"

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace growser {
namespace {

constexpr char kProfileFile[] = "Local State";
constexpr char kProfileContents[] = "the owner's real profile";

class GrowserUserDataDirTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    legacy_ = temp_dir_.GetPath().AppendASCII("Brave-Browser");
    target_ = temp_dir_.GetPath().AppendASCII("Growser");
  }

  // A legacy directory holding one file and one nested file: the shape of a
  // real profile in miniature.
  void GivenALegacyProfile() {
    ASSERT_TRUE(base::CreateDirectory(legacy_.AppendASCII("Default")));
    ASSERT_TRUE(
        base::WriteFile(legacy_.AppendASCII(kProfileFile), kProfileContents));
    ASSERT_TRUE(base::WriteFile(
        legacy_.AppendASCII("Default").AppendASCII("History"), "visited"));
  }

  std::string ContentsOf(const base::FilePath& path) {
    std::string contents;
    EXPECT_TRUE(base::ReadFileToString(path, &contents));
    return contents;
  }

  base::ScopedTempDir temp_dir_;
  base::FilePath legacy_;
  base::FilePath target_;
};

TEST_F(GrowserUserDataDirTest, TheLegacyDirectoryIsNotOursToTake) {
  GivenALegacyProfile();

  EXPECT_TRUE(MigrateUserDataDir(legacy_, target_));

  EXPECT_EQ(kProfileContents, ContentsOf(target_.AppendASCII(kProfileFile)));
  EXPECT_EQ("visited",
            ContentsOf(target_.AppendASCII("Default").AppendASCII("History")));

  // The reason this file exists. Everything above passes for a base::Move,
  // and a move takes the directory away from an installed Brave that reads
  // exactly this path.
  EXPECT_TRUE(base::PathExists(legacy_.AppendASCII(kProfileFile)));
  EXPECT_TRUE(
      base::PathExists(legacy_.AppendASCII("Default").AppendASCII("History")));
}

TEST_F(GrowserUserDataDirTest, WhatBelongsToARunningBrowserIsLeftBehind) {
  GivenALegacyProfile();
  // SingletonLock is a symlink whose target is a "hostname-pid" string rather
  // than a path that exists. It is present for as long as a browser is
  // running on this profile, and after a crash.
  ASSERT_TRUE(base::CreateSymbolicLink(
      base::FilePath(FILE_PATH_LITERAL("somehost-12345")),
      legacy_.AppendASCII("SingletonLock")));

  EXPECT_TRUE(MigrateUserDataDir(legacy_, target_));

  EXPECT_EQ(kProfileContents, ContentsOf(target_.AppendASCII(kProfileFile)));
  EXPECT_FALSE(base::PathExists(target_.AppendASCII("SingletonLock")));
}

TEST_F(GrowserUserDataDirTest, AProfileInUseIsNeverWrittenOver) {
  GivenALegacyProfile();
  ASSERT_TRUE(base::CreateDirectory(target_));
  ASSERT_TRUE(
      base::WriteFile(target_.AppendASCII(kProfileFile), "already running"));

  EXPECT_FALSE(MigrateUserDataDir(legacy_, target_));

  EXPECT_EQ("already running", ContentsOf(target_.AppendASCII(kProfileFile)));
}

TEST_F(GrowserUserDataDirTest, NothingToCarryLeavesNothingBehind) {
  // A fresh install on a machine that never had the old directory. The
  // migration must not report that it did anything, and must not create the
  // target - the browser creates that itself, moments later.
  EXPECT_FALSE(MigrateUserDataDir(legacy_, target_));

  EXPECT_FALSE(base::PathExists(target_));
}

TEST_F(GrowserUserDataDirTest, AnInterruptedCopyIsNotMistakenForAProfile) {
  GivenALegacyProfile();
  // What a migration killed halfway through leaves behind.
  const base::FilePath staging(target_.value() + ".migrating");
  ASSERT_TRUE(base::CreateDirectory(staging));
  ASSERT_TRUE(base::WriteFile(staging.AppendASCII(kProfileFile), "half"));

  EXPECT_TRUE(MigrateUserDataDir(legacy_, target_));

  EXPECT_EQ(kProfileContents, ContentsOf(target_.AppendASCII(kProfileFile)));
  EXPECT_FALSE(base::PathExists(staging));
}

TEST_F(GrowserUserDataDirTest, TheOldNameCarriesTheChannelSuffix) {
  const base::FilePath app_data = temp_dir_.GetPath();
  const base::FilePath legacy_root = app_data.AppendASCII("BraveSoftware");

  EXPECT_EQ(legacy_root.AppendASCII("Brave-Browser"),
            LegacyUserDataDirFor(app_data.AppendASCII("Growser")));
  EXPECT_EQ(legacy_root.AppendASCII("Brave-Browser-Development"),
            LegacyUserDataDirFor(app_data.AppendASCII("Growser-Development")));
  EXPECT_EQ(legacy_root.AppendASCII("Brave-Browser-Beta"),
            LegacyUserDataDirFor(app_data.AppendASCII("Growser-Beta")));
}

TEST_F(GrowserUserDataDirTest, ADirectoryThatIsNotOursIsNotGuessedAt) {
  // A user-chosen CrProductDirName, or a later rename that forgets this code.
  // Answering with a path anyway would be a guess about somebody else's data.
  EXPECT_TRUE(LegacyUserDataDirFor(temp_dir_.GetPath().AppendASCII("Chromium"))
                  .empty());
  // And that empty answer, handed back in, must do nothing rather than treat
  // the empty path as a directory.
  EXPECT_FALSE(MigrateUserDataDir(base::FilePath(), target_));
}

}  // namespace
}  // namespace growser
