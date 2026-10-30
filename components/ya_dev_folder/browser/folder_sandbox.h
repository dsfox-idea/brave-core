// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#ifndef BRAVE_COMPONENTS_YA_DEV_FOLDER_BROWSER_FOLDER_SANDBOX_H_
#define BRAVE_COMPONENTS_YA_DEV_FOLDER_BROWSER_FOLDER_SANDBOX_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/types/expected.h"
#include "brave/components/ya_dev_folder/common/ya_dev_folder.mojom.h"

namespace ya_dev_folder {

// A reply that carries only an error code (kOk for the calls with no payload).
mojom::ReplyPtr MakeReply(mojom::ErrorCode error);

// Largest file read or written in one call. A page gets whole files, so this
// is what keeps a mistake from asking the browser for gigabytes.
inline constexpr uint64_t kMaxFileBytes = 256ull * 1024 * 1024;

// The file operations on one folder. Every path is relative to the folder and
// uses '/'. The ONLY rules are: stay inside the folder (no absolute path, no
// "..", no drive letter or stream name, no Windows device name) and touch no
// symbolic link or reparse point, on the path or inside a tree. Everything
// else the user's account may do, a page may do - that is the experiment.
//
// Blocking: call on a thread that may block. A symlink swapped in between the
// check and the operation is not defended against; this is experiment code.
class FolderSandbox {
 public:
  explicit FolderSandbox(base::FilePath root);

  mojom::ReplyPtr List(const std::string& path) const;
  mojom::ReplyPtr Stat(const std::string& path) const;
  mojom::ReplyPtr Read(const std::string& path) const;
  mojom::ReplyPtr Write(const std::string& path,
                        base::span<const uint8_t> data) const;
  mojom::ReplyPtr MakeDirectory(const std::string& path) const;
  mojom::ReplyPtr Remove(const std::string& path, bool recursive) const;
  mojom::ReplyPtr Move(const std::string& from, const std::string& to) const;
  mojom::ReplyPtr Copy(const std::string& from, const std::string& to) const;

  // Exposed for tests.
  base::expected<base::FilePath, mojom::ErrorCode> Resolve(
      const std::string& path) const;

 private:
  base::FilePath root_;
};

}  // namespace ya_dev_folder

#endif  // BRAVE_COMPONENTS_YA_DEV_FOLDER_BROWSER_FOLDER_SANDBOX_H_
