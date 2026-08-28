/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/local_data_files_service.h"

#include "brave/components/brave_component_updater/browser/local_data_files_observer.h"

#include <array>
#include <string>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/functional/bind.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

using brave_component_updater::BraveComponent;

namespace {

// growser (#87): the files the Local Data Updater would have delivered, and
// the layout its consumers expect - each of them reads
// <install_dir>/1/<file>, so the version directory is part of the contract
// rather than a detail.
constexpr char kBundledDirName[] = "growser_local_data";
constexpr char kBundledVersionDir[] = "1";

struct BundledFile {
  int resource_id;
  const char* name;
};

constexpr auto kBundledFiles = std::to_array<BundledFile>({
    {IDR_LOCAL_DATA_DEBOUNCE_JSON, "debounce.json"},
    {IDR_LOCAL_DATA_CLEAN_URLS_JSON, "clean-urls.json"},
    {IDR_LOCAL_DATA_CLEAN_URLS_PERMISSIONS_JSON,
     "clean-urls-permissions.json"},
#if BUILDFLAG(ENABLE_REQUEST_OTR)
    {IDR_LOCAL_DATA_REQUEST_OTR_JSON, "request-otr.json"},
#endif
    {IDR_LOCAL_DATA_WEBCOMPAT_EXCEPTIONS_JSON, "webcompat-exceptions.json"},
    {IDR_LOCAL_DATA_HTTPS_UPGRADE_EXCEPTIONS_TXT,
     "https-upgrade-exceptions-list.txt"},
});

// Writes the bundled payload where a component would have been unpacked and
// returns the directory to announce - empty if anything went wrong, because a
// half-written directory would leave consumers reading files that are not
// there and reporting nothing.
base::FilePath WriteBundledLocalData() {
  base::FilePath root;
  if (!base::PathService::Get(component_updater::DIR_COMPONENT_USER, &root)) {
    LOG(ERROR) << "bundled local data: no component directory";
    return base::FilePath();
  }
  const base::FilePath install_dir = root.AppendASCII(kBundledDirName);
  const base::FilePath version_dir =
      install_dir.AppendASCII(kBundledVersionDir);
  if (!base::CreateDirectory(version_dir)) {
    LOG(ERROR) << "bundled local data: cannot create " << version_dir;
    return base::FilePath();
  }
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  for (const auto& file : kBundledFiles) {
    const std::string data = bundle.LoadDataResourceString(file.resource_id);
    if (data.empty()) {
      LOG(ERROR) << "bundled local data: resource " << file.resource_id
                 << " (" << file.name << ") is empty";
      return base::FilePath();
    }
    if (!base::WriteFile(version_dir.AppendASCII(file.name), data)) {
      LOG(ERROR) << "bundled local data: cannot write " << file.name;
      return base::FilePath();
    }
  }
  VLOG(1) << "bundled local data laid out in " << install_dir;
  return install_dir;
}

}  // namespace

namespace brave_component_updater {

LocalDataFilesService::LocalDataFilesService(BraveComponent::Delegate* delegate)
  : BraveComponent(delegate),
    initialized_(false) {}

LocalDataFilesService::~LocalDataFilesService() {
  for (auto& observer : observers_)
    observer.OnLocalDataFilesServiceDestroyed();
}

bool LocalDataFilesService::Start() {
  if (initialized_)
    return true;
  Register(kLocalDataFilesComponentName, kLocalDataFilesComponentId,
           kLocalDataFilesComponentBase64PublicKey);
  initialized_ = true;
  UseBundledDataUnlessComponentArrives();
  return true;
}

void LocalDataFilesService::UseBundledDataUnlessComponentArrives() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(&WriteBundledLocalData),
      base::BindOnce(&LocalDataFilesService::OnBundledDataWritten,
                     weak_factory_.GetWeakPtr()));
}

void LocalDataFilesService::OnBundledDataWritten(
    const base::FilePath& install_dir) {
  if (install_dir.empty() || component_ready_) {
    VLOG(1) << "bundled local data: not announced (empty="
            << install_dir.empty() << ", component=" << component_ready_
            << ")";
    return;
  }
  OnComponentReady(kLocalDataFilesComponentId, install_dir, std::string());
}

void LocalDataFilesService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  component_ready_ = true;
  for (auto& observer : observers_)
    observer.OnComponentReady(component_id, install_dir, manifest);
}

void LocalDataFilesService::AddObserver(LocalDataFilesObserver* observer) {
  observers_.AddObserver(observer);
}

void LocalDataFilesService::RemoveObserver(LocalDataFilesObserver* observer) {
  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<LocalDataFilesService>
LocalDataFilesServiceFactory(BraveComponent::Delegate* delegate) {
  return std::make_unique<LocalDataFilesService>(delegate);
}

}  // namespace brave_component_updater
