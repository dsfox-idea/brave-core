// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include "brave/browser/ya_dev_folder/ya_dev_folder_store.h"

#include <memory>
#include <utility>

#include "content/public/browser/browser_context.h"

namespace ya_dev_folder {

namespace {
const char kStoreKey[] = "ya_dev_folder_store";
}  // namespace

// static
YaDevFolderStore& YaDevFolderStore::Get(content::BrowserContext* context) {
  auto* store =
      static_cast<YaDevFolderStore*>(context->GetUserData(kStoreKey));
  if (!store) {
    auto created = std::make_unique<YaDevFolderStore>();
    store = created.get();
    context->SetUserData(kStoreKey, std::move(created));
  }
  return *store;
}

YaDevFolderStore::YaDevFolderStore() = default;
YaDevFolderStore::~YaDevFolderStore() = default;

std::optional<base::FilePath> YaDevFolderStore::Find(
    const url::Origin& origin) const {
  auto found = folders_.find(origin);
  if (found == folders_.end()) {
    return std::nullopt;
  }
  return found->second;
}

void YaDevFolderStore::Link(const url::Origin& origin, base::FilePath folder) {
  folders_[origin] = std::move(folder);
}

void YaDevFolderStore::Unlink(const url::Origin& origin) {
  folders_.erase(origin);
}

}  // namespace ya_dev_folder
