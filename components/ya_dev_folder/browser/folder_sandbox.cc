// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include "brave/components/ya_dev_folder/browser/folder_sandbox.h"

#include <optional>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/types/expected_macros.h"
#include "build/build_config.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace ya_dev_folder {

namespace {

using mojom::ErrorCode;

mojom::EntryPtr MakeEntry(const std::string& name,
                          bool is_directory,
                          int64_t size,
                          base::Time modified) {
  auto entry = mojom::Entry::New();
  entry->name = name;
  entry->is_directory = is_directory;
  entry->size = is_directory ? 0 : static_cast<uint64_t>(size);
  entry->modified_ms = modified.InMillisecondsFSinceUnixEpoch();
  return entry;
}

bool HasLinkInside(const base::FilePath& dir) {
  int types = base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES;
#if BUILDFLAG(IS_POSIX)
  types |= base::FileEnumerator::SHOW_SYM_LINKS;
#endif
  base::FileEnumerator walker(dir, /*recursive=*/true, types);
  for (base::FilePath item = walker.Next(); !item.empty();
       item = walker.Next()) {
    if (base::IsLink(item)) {
      return true;
    }
  }
  return false;
}

}  // namespace

mojom::ReplyPtr MakeReply(mojom::ErrorCode error) {
  auto reply = mojom::Reply::New();
  reply->error = error;
  return reply;
}

FolderSandbox::FolderSandbox(base::FilePath root) : root_(std::move(root)) {}

base::expected<base::FilePath, ErrorCode> FolderSandbox::Resolve(
    const std::string& path) const {
  if (base::IsLink(root_)) {
    return base::unexpected(ErrorCode::kSymlink);
  }
  base::FilePath resolved = root_;
  if (path.empty()) {
    return resolved;
  }
  if (path.front() == '/' || path.find('\\') != std::string::npos ||
      path.find('\0') != std::string::npos) {
    return base::unexpected(ErrorCode::kInvalidPath);
  }
  for (const std::string& part : base::SplitString(
           path, "/", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
    // ':' is a drive letter or an NTFS stream on Windows; no folder needs it.
    // A trailing dot or space is stripped by Win32, so "..." and "a." would
    // name something other than what was checked.
    if (part.empty() || part.front() == ' ' || part.back() == ' ' ||
        part.back() == '.' || part.find(':') != std::string::npos) {
      return base::unexpected(ErrorCode::kInvalidPath);
    }
    const base::FilePath name = base::FilePath::FromUTF8Unsafe(part);
    if (name.empty() || name.IsAbsolute() || name.ReferencesParent() ||
        base::IsReservedNameOnWindows(name.value())) {
      return base::unexpected(ErrorCode::kInvalidPath);
    }
    resolved = resolved.Append(name);
    if (base::IsLink(resolved)) {
      return base::unexpected(ErrorCode::kSymlink);
    }
  }
  return resolved;
}

mojom::ReplyPtr FolderSandbox::List(const std::string& path) const {
  ASSIGN_OR_RETURN(base::FilePath dir, Resolve(path), &MakeReply);
  if (!base::PathExists(dir)) {
    return MakeReply(ErrorCode::kNotFound);
  }
  if (!base::DirectoryExists(dir)) {
    return MakeReply(ErrorCode::kNotADirectory);
  }
  auto reply = MakeReply(ErrorCode::kOk);
  reply->entries.emplace();
  int types = base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES;
#if BUILDFLAG(IS_POSIX)
  types |= base::FileEnumerator::SHOW_SYM_LINKS;
#endif
  base::FileEnumerator walker(dir, /*recursive=*/false, types);
  for (base::FilePath item = walker.Next(); !item.empty();
       item = walker.Next()) {
    // A link is listed nowhere: the page cannot be led through one.
    if (base::IsLink(item)) {
      continue;
    }
    const base::FileEnumerator::FileInfo info = walker.GetInfo();
    reply->entries->push_back(MakeEntry(item.BaseName().AsUTF8Unsafe(),
                                        info.IsDirectory(), info.GetSize(),
                                        info.GetLastModifiedTime()));
  }
  return reply;
}

mojom::ReplyPtr FolderSandbox::Stat(const std::string& path) const {
  ASSIGN_OR_RETURN(base::FilePath target, Resolve(path), &MakeReply);
  base::File::Info info;
  if (!base::GetFileInfo(target, &info)) {
    return MakeReply(ErrorCode::kNotFound);
  }
  auto reply = MakeReply(ErrorCode::kOk);
  reply->entry = MakeEntry(target.BaseName().AsUTF8Unsafe(), info.is_directory,
                           info.size, info.last_modified);
  return reply;
}

mojom::ReplyPtr FolderSandbox::Read(const std::string& path) const {
  ASSIGN_OR_RETURN(base::FilePath file, Resolve(path), &MakeReply);
  base::File::Info info;
  if (!base::GetFileInfo(file, &info)) {
    return MakeReply(ErrorCode::kNotFound);
  }
  if (info.is_directory) {
    return MakeReply(ErrorCode::kIsADirectory);
  }
  if (static_cast<uint64_t>(info.size) > kMaxFileBytes) {
    return MakeReply(ErrorCode::kTooLarge);
  }
  std::optional<std::vector<uint8_t>> bytes = base::ReadFileToBytes(file);
  if (!bytes) {
    return MakeReply(ErrorCode::kIo);
  }
  auto reply = MakeReply(ErrorCode::kOk);
  reply->data = mojo_base::BigBuffer(*bytes);
  return reply;
}

mojom::ReplyPtr FolderSandbox::Write(const std::string& path,
                                     base::span<const uint8_t> data) const {
  ASSIGN_OR_RETURN(base::FilePath file, Resolve(path), &MakeReply);
  if (file == root_ || base::DirectoryExists(file)) {
    return MakeReply(ErrorCode::kIsADirectory);
  }
  if (data.size() > kMaxFileBytes) {
    return MakeReply(ErrorCode::kTooLarge);
  }
  if (!base::CreateDirectory(file.DirName()) || !base::WriteFile(file, data)) {
    return MakeReply(ErrorCode::kIo);
  }
  return MakeReply(ErrorCode::kOk);
}

mojom::ReplyPtr FolderSandbox::MakeDirectory(const std::string& path) const {
  ASSIGN_OR_RETURN(base::FilePath dir, Resolve(path), &MakeReply);
  if (base::PathExists(dir) && !base::DirectoryExists(dir)) {
    return MakeReply(ErrorCode::kAlreadyExists);
  }
  return MakeReply(base::CreateDirectory(dir) ? ErrorCode::kOk
                                              : ErrorCode::kIo);
}

mojom::ReplyPtr FolderSandbox::Remove(const std::string& path,
                                      bool recursive) const {
  ASSIGN_OR_RETURN(base::FilePath target, Resolve(path), &MakeReply);
  if (target == root_) {
    return MakeReply(ErrorCode::kInvalidPath);
  }
  if (!base::PathExists(target)) {
    return MakeReply(ErrorCode::kNotFound);
  }
  if (base::DirectoryExists(target)) {
    if (!recursive && !base::IsDirectoryEmpty(target)) {
      return MakeReply(ErrorCode::kIo);
    }
    if (recursive && HasLinkInside(target)) {
      return MakeReply(ErrorCode::kSymlink);
    }
    return MakeReply(base::DeletePathRecursively(target) ? ErrorCode::kOk
                                                         : ErrorCode::kIo);
  }
  return MakeReply(base::DeleteFile(target) ? ErrorCode::kOk : ErrorCode::kIo);
}

mojom::ReplyPtr FolderSandbox::Move(const std::string& from,
                                    const std::string& to) const {
  ASSIGN_OR_RETURN(base::FilePath source, Resolve(from), &MakeReply);
  ASSIGN_OR_RETURN(base::FilePath target, Resolve(to), &MakeReply);
  if (source == root_ || target == root_ || source == target ||
      source.IsParent(target)) {
    return MakeReply(ErrorCode::kInvalidPath);
  }
  if (!base::PathExists(source)) {
    return MakeReply(ErrorCode::kNotFound);
  }
  if (base::PathExists(target)) {
    return MakeReply(ErrorCode::kAlreadyExists);
  }
  if (base::DirectoryExists(source) && HasLinkInside(source)) {
    return MakeReply(ErrorCode::kSymlink);
  }
  if (!base::CreateDirectory(target.DirName()) || !base::Move(source, target)) {
    return MakeReply(ErrorCode::kIo);
  }
  return MakeReply(ErrorCode::kOk);
}

mojom::ReplyPtr FolderSandbox::Copy(const std::string& from,
                                    const std::string& to) const {
  ASSIGN_OR_RETURN(base::FilePath source, Resolve(from), &MakeReply);
  ASSIGN_OR_RETURN(base::FilePath target, Resolve(to), &MakeReply);
  if (target == root_ || source == target || source.IsParent(target)) {
    return MakeReply(ErrorCode::kInvalidPath);
  }
  if (!base::PathExists(source)) {
    return MakeReply(ErrorCode::kNotFound);
  }
  if (base::PathExists(target)) {
    return MakeReply(ErrorCode::kAlreadyExists);
  }
  const bool directory = base::DirectoryExists(source);
  if (directory && HasLinkInside(source)) {
    return MakeReply(ErrorCode::kSymlink);
  }
  if (!base::CreateDirectory(target.DirName())) {
    return MakeReply(ErrorCode::kIo);
  }
  const bool copied = directory
                          ? base::CopyDirectory(source, target, true)
                          : base::CopyFile(source, target);
  return MakeReply(copied ? ErrorCode::kOk : ErrorCode::kIo);
}

}  // namespace ya_dev_folder
