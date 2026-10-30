// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#ifndef BRAVE_BROWSER_YA_DEV_FOLDER_YA_DEV_FOLDER_STORE_H_
#define BRAVE_BROWSER_YA_DEV_FOLDER_YA_DEV_FOLDER_STORE_H_

#include <map>
#include <optional>

#include "base/files/file_path.h"
#include "base/supports_user_data.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
}

namespace ya_dev_folder {

// The folder each origin has linked, for the life of one browser profile. In
// memory only: a restart forgets every link, which is the safe default for an
// experiment that hands a page the whole folder.
class YaDevFolderStore : public base::SupportsUserData::Data {
 public:
  static YaDevFolderStore& Get(content::BrowserContext* context);

  YaDevFolderStore();
  ~YaDevFolderStore() override;

  std::optional<base::FilePath> Find(const url::Origin& origin) const;
  void Link(const url::Origin& origin, base::FilePath folder);
  void Unlink(const url::Origin& origin);

 private:
  std::map<url::Origin, base::FilePath> folders_;
};

}  // namespace ya_dev_folder

#endif  // BRAVE_BROWSER_YA_DEV_FOLDER_YA_DEV_FOLDER_STORE_H_
