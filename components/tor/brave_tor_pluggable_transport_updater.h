/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_BRAVE_TOR_PLUGGABLE_TRANSPORT_UPDATER_H_
#define BRAVE_COMPONENTS_TOR_BRAVE_TOR_PLUGGABLE_TRANSPORT_UPDATER_H_

#include <cstdint>
#include <iterator>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "build/build_config.h"
#include "crypto/sha2.h"

class PrefService;

using brave_component_updater::BraveComponent;

namespace tor {

extern const char kSnowflakeExecutableName[];
extern const char kObfs4ExecutableName[];

#if BUILDFLAG(IS_WIN)
// Growser-157: our key, not Brave's - id pbbiomhpfnaklbbjmaciapdcnmhhndcd.
inline constexpr uint8_t kTorPluggableTransportComponentPublicKeySHA256[32] = {
    0xf1, 0x18, 0xec, 0x7f, 0x5d, 0x0a, 0xb1, 0x19, 0xc0, 0x28, 0x0f,
    0x32, 0xdc, 0x77, 0xd3, 0x23, 0xcc, 0x5d, 0xba, 0x5a, 0x11, 0x78,
    0x9a, 0x6d, 0x0d, 0x4a, 0xad, 0xd9, 0xd6, 0xc6, 0x86, 0x8b};
static_assert(std::size(kTorPluggableTransportComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_MAC)
// Growser-163: our key, not Brave's - id kimmbaggbekmbachcdhfefjkkkibhfda.
inline constexpr uint8_t kTorPluggableTransportComponentPublicKeySHA256[32] = {
    0xa8, 0xcc, 0x10, 0x66, 0x14, 0xac, 0x10, 0x27, 0x23, 0x75, 0x45,
    0x9a, 0xaa, 0x81, 0x75, 0x30, 0x1e, 0x80, 0x77, 0xd6, 0x15, 0xa3,
    0x9b, 0x11, 0x07, 0x63, 0xcc, 0x09, 0xc7, 0x60, 0xa4, 0xa1};
static_assert(std::size(kTorPluggableTransportComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_LINUX)
// Growser-202: our key, not Brave's - id ppljljnkhpkehiacdkdalonjdidcbffk.
inline constexpr uint8_t kTorPluggableTransportComponentPublicKeySHA256[32] = {
    0xff, 0xb9, 0xb9, 0xda, 0x7f, 0xa4, 0x78, 0x02, 0x3a, 0x30, 0xbe,
    0xd9, 0x38, 0x32, 0x15, 0x5a, 0x82, 0xb4, 0xfa, 0xe3, 0x72, 0x0e,
    0x1f, 0x4f, 0xc9, 0x93, 0x93, 0xac, 0x44, 0x2d, 0xa8, 0x79};
static_assert(std::size(kTorPluggableTransportComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#endif

class BraveTorPluggableTransportUpdater : public BraveComponent {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPluggableTransportReady(bool success) {}

   protected:
    ~Observer() override = default;
  };

  BraveTorPluggableTransportUpdater(
      BraveComponent::Delegate* component_delegate,
      PrefService* local_state,
      const base::FilePath& user_data_dir);
  BraveTorPluggableTransportUpdater(const BraveTorPluggableTransportUpdater&) =
      delete;
  BraveTorPluggableTransportUpdater& operator=(
      const BraveTorPluggableTransportUpdater&) = delete;
  ~BraveTorPluggableTransportUpdater() override;

  void Register();
  void Unregister();
  void Cleanup();

  bool IsReady() const;
  const base::FilePath& GetSnowflakeExecutable() const;
  const base::FilePath& GetObfs4Executable() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void OnComponentReady(const std::string& component_id,
                        const base::FilePath& install_dir,
                        const std::string& manifest) override;

 private:
  void OnInitialized(const base::FilePath& install_dir, bool succeeded);

  bool registered_ = false;
  bool is_ready_ = false;

  base::ObserverList<Observer> observers_;
  raw_ptr<PrefService> local_state_ = nullptr;
  base::FilePath user_data_dir_;
  base::FilePath snowflake_path_;  // Relative to the user data dir
  base::FilePath obfs4_path_;      // Relative to the user data dir

  base::WeakPtrFactory<BraveTorPluggableTransportUpdater> weak_ptr_factory_{
      this};
};

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_BRAVE_TOR_PLUGGABLE_TRANSPORT_UPDATER_H_
