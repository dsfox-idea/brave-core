// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_default_resource_provider.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

constexpr char kAdBlockResourcesFilename[] = "resources.json";

// growser (#87): the copy that ships with the build. Reading it is blocking
// work, so it runs on the thread pool like the component file does.
std::string BundledResourcesJson() {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
      IDR_ADBLOCK_RESOURCES_JSON);
}

}  // namespace

namespace brave_shields {

AdBlockDefaultResourceProvider::AdBlockDefaultResourceProvider(
    component_updater::ComponentUpdateService* cus) {
  // Can be nullptr in unit tests
  if (!cus) {
    return;
  }

  RegisterAdBlockDefaultResourceComponent(
      cus,
      base::BindRepeating(&AdBlockDefaultResourceProvider::OnComponentReady,
                          weak_factory_.GetWeakPtr()));

  // The component is served by go-updater.brave.com, which answers a fork
  // "403 Missing auth header", so it never becomes ready and every ##+js()
  // rule stays inert. Serve the bundled resources now; if the component ever
  // does arrive, OnComponentReady replaces them.
  LoadBundledResources();
}

AdBlockDefaultResourceProvider::~AdBlockDefaultResourceProvider() = default;

base::FilePath AdBlockDefaultResourceProvider::GetResourcesPath() {
  if (component_path_.empty()) {
    // Since we know it's empty return it as is.
    return component_path_;
  }

  return component_path_.AppendASCII(kAdBlockResourcesFilename);
}

void AdBlockDefaultResourceProvider::OnComponentReady(
    const base::FilePath& path) {
  component_path_ = path;
  base::FilePath resources_path = GetResourcesPath();

  if (resources_path.empty()) {
    // This should not happen, but if it does, we should not proceed.
    return;
  }

  // Load the resources (as ResourceStorage)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     resources_path),
      base::BindOnce(
          [](base::WeakPtr<AdBlockDefaultResourceProvider> provider,
             const std::string& resources_json) {
            if (!provider) {
              return;
            }
            auto storage = adblock::new_resource_storage(resources_json);
            provider->NotifyResourcesLoaded(std::move(storage));
          },
          weak_factory_.GetWeakPtr()));
}

void AdBlockDefaultResourceProvider::LoadBundledResources() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&BundledResourcesJson),
      base::BindOnce(&AdBlockDefaultResourceProvider::OnBundledResourcesRead,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockDefaultResourceProvider::OnBundledResourcesRead(
    const std::string& resources_json) {
  if (!component_path_.empty()) {
    // A component arrived while this was being read; it wins.
    return;
  }
  NotifyResourcesLoaded(adblock::new_resource_storage(resources_json));
}

void AdBlockDefaultResourceProvider::LoadResources(
    base::OnceCallback<void(AdblockResourceStorageBox)> cb) {
  base::FilePath resources_path = GetResourcesPath();
  if (resources_path.empty()) {
    // No component - which for a fork is the permanent state - so answer with
    // the bundled resources rather than the empty storage this used to return.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()}, base::BindOnce(&BundledResourcesJson),
        base::BindOnce(
            [](base::OnceCallback<void(AdblockResourceStorageBox)> cb,
               const std::string& resources_json) {
              std::move(cb).Run(
                  adblock::new_resource_storage(resources_json));
            },
            std::move(cb)));
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     resources_path),
      base::BindOnce(
          [](base::OnceCallback<void(AdblockResourceStorageBox)> cb,
             const std::string& resources_json) {
            auto storage = adblock::new_resource_storage(resources_json);
            std::move(cb).Run(std::move(storage));
          },
          std::move(cb)));
}

}  // namespace brave_shields
