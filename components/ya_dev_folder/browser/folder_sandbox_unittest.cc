// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include "brave/components/ya_dev_folder/browser/folder_sandbox.h"

#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/process/launch.h"
#include "base/strings/string_view_util.h"
#include "build/build_config.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ya_dev_folder {

namespace {

using mojom::ErrorCode;

std::vector<uint8_t> Bytes(const std::string& text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

// A symbolic link on POSIX; a junction on Windows, which needs no privilege
// and is a reparse point the sandbox must refuse just the same.
bool MakeLink(const base::FilePath& target, const base::FilePath& link) {
#if BUILDFLAG(IS_WIN)
  base::CommandLine command(base::FilePath(FILE_PATH_LITERAL("cmd.exe")));
  command.AppendArg("/c");
  command.AppendArg("mklink");
  command.AppendArg("/J");
  command.AppendArgPath(link);
  command.AppendArgPath(target);
  std::string output;
  return base::GetAppOutput(command, &output) && base::DirectoryExists(link);
#else
  return base::CreateSymbolicLink(target, link);
#endif
}

}  // namespace

class FolderSandboxTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_.CreateUniqueTempDir());
    root_ = temp_.GetPath().AppendASCII("linked");
    ASSERT_TRUE(base::CreateDirectory(root_));
    sandbox_ = std::make_unique<FolderSandbox>(root_);
  }

  std::string Contents(const std::string& path) {
    std::string text;
    EXPECT_TRUE(base::ReadFileToString(
        root_.Append(base::FilePath::FromUTF8Unsafe(path)), &text));
    return text;
  }

  base::ScopedTempDir temp_;
  base::FilePath root_;
  std::unique_ptr<FolderSandbox> sandbox_;
};

TEST_F(FolderSandboxTest, RefusesEveryWayOutOfTheFolder) {
  for (const char* path :
       {"..", "../x", "a/../../x", "/abs", "C:/x", "c:x", "a\\b", "a//b",
        "a/./b", "x:stream", "trailing.", "trailing ", " leading", "con",
        "aux.txt", "a/nul", "a/..."}) {
    EXPECT_EQ(sandbox_->Stat(path)->error, ErrorCode::kInvalidPath) << path;
    EXPECT_EQ(sandbox_->Write(path, Bytes("x"))->error,
              ErrorCode::kInvalidPath)
        << path;
  }
  EXPECT_FALSE(base::PathExists(temp_.GetPath().AppendASCII("x")));
}

TEST_F(FolderSandboxTest, EmbeddedNulIsRefused) {
  EXPECT_EQ(sandbox_->Stat(std::string("a\0b", 3))->error,
            ErrorCode::kInvalidPath);
}

TEST_F(FolderSandboxTest, WriteReadRoundTripAndOverwrite) {
  EXPECT_EQ(sandbox_->Write("dir/sub/a.txt", Bytes("hello"))->error,
            ErrorCode::kOk);
  EXPECT_EQ(Contents("dir/sub/a.txt"), "hello");

  auto read = sandbox_->Read("dir/sub/a.txt");
  ASSERT_EQ(read->error, ErrorCode::kOk);
  ASSERT_TRUE(read->data);
  EXPECT_EQ(std::string(base::as_string_view(read->data->byte_span())),
            "hello");

  EXPECT_EQ(sandbox_->Write("dir/sub/a.txt", Bytes(""))->error,
            ErrorCode::kOk);
  EXPECT_EQ(Contents("dir/sub/a.txt"), "");
}

TEST_F(FolderSandboxTest, BinaryAndNonAsciiNames) {
  const std::vector<uint8_t> binary = {0, 1, 2, 0xff, 0, 0x80};
  const std::string name = "\xD0\xBF\xD0\xB0\xD0\xBF\xD0\xBA\xD0\xB0/\xE2\x82\xAC.bin";
  EXPECT_EQ(sandbox_->Write(name, binary)->error, ErrorCode::kOk);
  auto read = sandbox_->Read(name);
  ASSERT_EQ(read->error, ErrorCode::kOk);
  EXPECT_EQ(std::vector<uint8_t>(read->data->byte_span().begin(),
                                 read->data->byte_span().end()),
            binary);
}

TEST_F(FolderSandboxTest, ListStatAndKinds) {
  ASSERT_EQ(sandbox_->Write("a.txt", Bytes("abc"))->error, ErrorCode::kOk);
  ASSERT_EQ(sandbox_->MakeDirectory("d/e")->error, ErrorCode::kOk);

  auto list = sandbox_->List("");
  ASSERT_EQ(list->error, ErrorCode::kOk);
  ASSERT_EQ(list->entries->size(), 2u);
  for (const auto& entry : *list->entries) {
    if (entry->name == "a.txt") {
      EXPECT_FALSE(entry->is_directory);
      EXPECT_EQ(entry->size, 3u);
    } else {
      EXPECT_EQ(entry->name, "d");
      EXPECT_TRUE(entry->is_directory);
    }
  }

  auto stat = sandbox_->Stat("d/e");
  ASSERT_EQ(stat->error, ErrorCode::kOk);
  EXPECT_TRUE(stat->entry->is_directory);
  EXPECT_EQ(sandbox_->Stat("nope")->error, ErrorCode::kNotFound);
  EXPECT_EQ(sandbox_->List("nope")->error, ErrorCode::kNotFound);
  EXPECT_EQ(sandbox_->List("a.txt")->error, ErrorCode::kNotADirectory);
  EXPECT_EQ(sandbox_->Read("d")->error, ErrorCode::kIsADirectory);
  EXPECT_EQ(sandbox_->Write("d", Bytes("x"))->error, ErrorCode::kIsADirectory);
  EXPECT_EQ(sandbox_->Write("", Bytes("x"))->error, ErrorCode::kIsADirectory);
}

TEST_F(FolderSandboxTest, MakeDirectoryOverAFileFails) {
  ASSERT_EQ(sandbox_->Write("f", Bytes("x"))->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->MakeDirectory("f")->error, ErrorCode::kAlreadyExists);
  EXPECT_EQ(sandbox_->MakeDirectory("")->error, ErrorCode::kOk);
}

TEST_F(FolderSandboxTest, RemoveFileEmptyAndFullFolder) {
  ASSERT_EQ(sandbox_->Write("d/a", Bytes("x"))->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->Remove("d", false)->error, ErrorCode::kIo);
  EXPECT_TRUE(base::PathExists(root_.AppendASCII("d").AppendASCII("a")));
  EXPECT_EQ(sandbox_->Remove("d/a", false)->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->Remove("d", false)->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->Remove("d", false)->error, ErrorCode::kNotFound);

  ASSERT_EQ(sandbox_->Write("t/u/v", Bytes("x"))->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->Remove("t", true)->error, ErrorCode::kOk);
  EXPECT_FALSE(base::PathExists(root_.AppendASCII("t")));
}

TEST_F(FolderSandboxTest, TheLinkedFolderItselfCannotBeRemovedOrMoved) {
  EXPECT_EQ(sandbox_->Remove("", true)->error, ErrorCode::kInvalidPath);
  EXPECT_EQ(sandbox_->Move("", "x")->error, ErrorCode::kInvalidPath);
  EXPECT_EQ(sandbox_->Move("x", "")->error, ErrorCode::kInvalidPath);
  EXPECT_TRUE(base::DirectoryExists(root_));
}

TEST_F(FolderSandboxTest, MoveAndCopy) {
  ASSERT_EQ(sandbox_->Write("a/f.txt", Bytes("one"))->error, ErrorCode::kOk);
  ASSERT_EQ(sandbox_->Write("a/g.txt", Bytes("two"))->error, ErrorCode::kOk);

  EXPECT_EQ(sandbox_->Copy("a/f.txt", "b/f2.txt")->error, ErrorCode::kOk);
  EXPECT_EQ(Contents("b/f2.txt"), "one");
  EXPECT_EQ(Contents("a/f.txt"), "one");

  EXPECT_EQ(sandbox_->Copy("a", "c")->error, ErrorCode::kOk);
  EXPECT_EQ(Contents("c/g.txt"), "two");

  EXPECT_EQ(sandbox_->Move("a/f.txt", "m/f3.txt")->error, ErrorCode::kOk);
  EXPECT_FALSE(base::PathExists(root_.AppendASCII("a").AppendASCII("f.txt")));
  EXPECT_EQ(Contents("m/f3.txt"), "one");

  EXPECT_EQ(sandbox_->Move("c", "d")->error, ErrorCode::kOk);
  EXPECT_EQ(Contents("d/g.txt"), "two");
}

TEST_F(FolderSandboxTest, MoveAndCopyNeverOverwriteOrNestIntoThemselves) {
  ASSERT_EQ(sandbox_->Write("x", Bytes("1"))->error, ErrorCode::kOk);
  ASSERT_EQ(sandbox_->Write("y", Bytes("2"))->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->Move("x", "y")->error, ErrorCode::kAlreadyExists);
  EXPECT_EQ(sandbox_->Copy("x", "y")->error, ErrorCode::kAlreadyExists);
  EXPECT_EQ(Contents("y"), "2");
  EXPECT_EQ(sandbox_->Move("x", "x")->error, ErrorCode::kInvalidPath);

  ASSERT_EQ(sandbox_->MakeDirectory("p")->error, ErrorCode::kOk);
  EXPECT_EQ(sandbox_->Move("p", "p/q")->error, ErrorCode::kInvalidPath);
  EXPECT_EQ(sandbox_->Copy("p", "p/q")->error, ErrorCode::kInvalidPath);
  EXPECT_EQ(sandbox_->Move("missing", "z")->error, ErrorCode::kNotFound);
  EXPECT_EQ(sandbox_->Copy("missing", "z")->error, ErrorCode::kNotFound);
}

// Creating a symlink needs a privilege on Windows; without it the test says so
// instead of passing.
class FolderSandboxSymlinkTest : public FolderSandboxTest {
 protected:
  void SetUp() override {
    FolderSandboxTest::SetUp();
    outside_ = temp_.GetPath().AppendASCII("outside");
    ASSERT_TRUE(base::CreateDirectory(outside_));
    ASSERT_TRUE(base::WriteFile(outside_.AppendASCII("secret"), "s"));
    link_ = root_.AppendASCII("link");
    can_link_ = MakeLink(outside_, link_);
  }

  base::FilePath outside_;
  base::FilePath link_;
  bool can_link_ = false;
};

TEST_F(FolderSandboxSymlinkTest, NothingFollowsALink) {
  if (!can_link_) {
    GTEST_SKIP() << "cannot create links here";
  }
  EXPECT_EQ(sandbox_->Read("link/secret")->error, ErrorCode::kSymlink);
  EXPECT_EQ(sandbox_->Write("link/new", Bytes("x"))->error, ErrorCode::kSymlink);
  EXPECT_EQ(sandbox_->List("link")->error, ErrorCode::kSymlink);
  EXPECT_EQ(sandbox_->Stat("link")->error, ErrorCode::kSymlink);
  EXPECT_EQ(sandbox_->Remove("link", true)->error, ErrorCode::kSymlink);
  EXPECT_FALSE(base::PathExists(outside_.AppendASCII("new")));
  EXPECT_TRUE(base::PathExists(outside_.AppendASCII("secret")));
}

TEST_F(FolderSandboxSymlinkTest, ALinkIsNotListed) {
  if (!can_link_) {
    GTEST_SKIP() << "cannot create links here";
  }
  ASSERT_EQ(sandbox_->Write("real", Bytes("x"))->error, ErrorCode::kOk);
  auto list = sandbox_->List("");
  ASSERT_EQ(list->error, ErrorCode::kOk);
  ASSERT_EQ(list->entries->size(), 1u);
  EXPECT_EQ((*list->entries)[0]->name, "real");
}

TEST_F(FolderSandboxSymlinkTest, TreesHoldingALinkAreNotWalked) {
  if (!can_link_) {
    GTEST_SKIP() << "cannot create links here";
  }
  ASSERT_EQ(sandbox_->MakeDirectory("tree")->error, ErrorCode::kOk);
  ASSERT_TRUE(
      MakeLink(outside_, root_.AppendASCII("tree").AppendASCII("l")));
  EXPECT_EQ(sandbox_->Remove("tree", true)->error, ErrorCode::kSymlink);
  EXPECT_EQ(sandbox_->Copy("tree", "tree2")->error, ErrorCode::kSymlink);
  EXPECT_EQ(sandbox_->Move("tree", "tree3")->error, ErrorCode::kSymlink);
  EXPECT_TRUE(base::PathExists(outside_.AppendASCII("secret")));
  EXPECT_FALSE(base::PathExists(root_.AppendASCII("tree2")));
}

TEST_F(FolderSandboxSymlinkTest, ALinkedRootIsRefused) {
  if (!can_link_) {
    GTEST_SKIP() << "cannot create links here";
  }
  FolderSandbox through_link(link_);
  EXPECT_EQ(through_link.List("")->error, ErrorCode::kSymlink);
  EXPECT_EQ(through_link.Read("secret")->error, ErrorCode::kSymlink);
}

}  // namespace ya_dev_folder
