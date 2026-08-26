// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_filter_list_catalog_provider.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_shields/core/browser/ad_block_component_installer.h"
#include "components/grit/brave_components_resources.h"
#include "ui/base/resource/resource_bundle.h"

constexpr char kListCatalogFile[] = "list_catalog.json";

namespace {

// growser (#87): the catalogue that ships with the build. Its component is
// served by go-updater.brave.com, which answers a fork "403 Missing auth
// header", so without this the settings page has no filter lists to offer at
// all - and neither of the two ways a consumer asks for the catalogue (the
// pull below, or the push to observers) answers when no component is there.
const std::string& BundledCatalog() {
  static const base::NoDestructor<std::string> catalog([] {
    std::string json =
        ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
            IDR_ADBLOCK_LIST_CATALOG_JSON);
    if (json.empty()) {
      LOG(ERROR) << "bundled filter list catalogue is missing";
    }
    return json;
  }());
  return *catalog;
}

// Always on the next turn, never inline: observers are added while the things
// that own them are still being constructed, and a callback that re-enters a
// half-built object is its own bug.
//
// No current sequence means no task environment, which means a unit test that
// never expected a catalogue. Delivering there is not merely pointless - it
// crashes, because the consumer starts a RepeatingTimer on receipt and a timer
// needs a sequence too. The browser always has one.
void DeliverBundled(base::OnceClosure task) {
  if (!base::SequencedTaskRunner::HasCurrentDefault()) {
    return;
  }
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
                                                           std::move(task));
}

}  // namespace

namespace brave_shields {

AdBlockFilterListCatalogProvider::AdBlockFilterListCatalogProvider(
    component_updater::ComponentUpdateService* cus) {
  TRACE_EVENT("brave.adblock", "RegisterAdBlockFilterListCatalogComponent",
              perfetto::Flow::FromPointer(this));
  // Can be nullptr in unit tests
  if (cus) {
    RegisterAdBlockFilterListCatalogComponent(
        cus,
        base::BindRepeating(&AdBlockFilterListCatalogProvider::OnComponentReady,
                            weak_factory_.GetWeakPtr()));
  }

}

AdBlockFilterListCatalogProvider::~AdBlockFilterListCatalogProvider() = default;

void AdBlockFilterListCatalogProvider::AddObserver(
    AdBlockFilterListCatalogProvider::Observer* observer) {
  observers_.AddObserver(observer);

  // growser (#87): with no component there is nothing to wait for, so tell a
  // new observer what we have. Posted: observers are added during
  // construction of the things that own them, and calling back into a
  // half-built object is its own bug.
  if (component_path_.empty()) {
    DeliverBundled(
        base::BindOnce(&AdBlockFilterListCatalogProvider::NotifyBundledCatalog,
                       weak_factory_.GetWeakPtr(), observer));
  }
}

void AdBlockFilterListCatalogProvider::NotifyBundledCatalog(
    Observer* observer) {
  if (!component_path_.empty() || !observers_.HasObserver(observer) ||
      BundledCatalog().empty()) {
    return;
  }
  observer->OnFilterListCatalogLoaded(BundledCatalog());
}

void AdBlockFilterListCatalogProvider::RemoveObserver(
    AdBlockFilterListCatalogProvider::Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded(
    const std::string& catalog_json) {
  TRACE_EVENT("brave.adblock",
              "AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded",
              perfetto::TerminatingFlow::FromPointer(this), "catalog_json_size",
              catalog_json.size());
  for (auto& observer : observers_) {
    observer.OnFilterListCatalogLoaded(catalog_json);
  }
}

void AdBlockFilterListCatalogProvider::OnComponentReady(
    const base::FilePath& path) {
  TRACE_EVENT("brave.adblock",
              "AdBlockFilterListCatalogProvider::OnComponentReady",
              perfetto::Flow::FromPointer(this), "path", path.value());
  component_path_ = path;

  // Load the filter list catalog (as a string)
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kListCatalogFile)),
      base::BindOnce(
          &AdBlockFilterListCatalogProvider::OnFilterListCatalogLoaded,
          weak_factory_.GetWeakPtr()));
}

void AdBlockFilterListCatalogProvider::LoadFilterListCatalog(
    base::OnceCallback<void(const std::string& catalog_json)> cb) {
  if (component_path_.empty()) {
    // growser (#87): upstream returns here and waits for a component that,
    // for a fork, never comes. Answer with the bundled catalogue instead,
    // posted rather than run inline so the callback keeps its async contract.
    DeliverBundled(base::BindOnce(std::move(cb), BundledCatalog()));
    return;
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kListCatalogFile)),
      std::move(cb));
}

}  // namespace brave_shields
