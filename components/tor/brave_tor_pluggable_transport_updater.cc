/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/brave_tor_pluggable_transport_updater.h"

#include <string>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/task/task_traits.h"
#include "build/build_config.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
// Growser-157: ours, for the same reason as the client component.
constexpr const char kComponentName[] =
    "Growser Tor Pluggable Transports (Windows)";
constexpr const char kTorPluggableTransportComponentId[] =
    "pbbiomhpfnaklbbjmaciapdcnmhhndcd";
constexpr const char kComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAm8HIH0ZArglIkTPmf7iz"
    "VW9MBEdTlkFzXa2jc1hRdwyVWZ4IlwRyLO9SPmRxp5/gJP6XhAooAqKEeH6dC0Ed"
    "R1gKdpfxkOL4qNr09zT4fkhi2+nzZqGI1y+A55gF9epNLxrOHsWlw2X7oJX0omJU"
    "GFEV9yau6RdYicPmAE/QMtk025N8GtCtO0zAILvb9P4gP2T6aTsKGC/UmYYQlnTC"
    "LEeK82jy1y1cQ03gOE9gOnmTV5L+8rUcWJzP/zeg83ky3iYt2Dgs6kT4zCPILBrX"
    "WBwE6v2rX5tgz+NIYQ1vzSwY5fJMsRlKuaPswzTXotX+VfsIpRBvMXMeHhOMIhxH"
    "BwIDAQAB";
#elif BUILDFLAG(IS_MAC)
// Growser-163: our own component, with a key of its own. The id is derived
// from the key, so these two lines cannot disagree without the updater
// refusing whatever it downloads. A pair per platform is what upstream does
// and what we need: sharing the Windows id would give two platforms the same
// appid for different bytes, and the Worker filters by appid.
constexpr const char kComponentName[] =
    "Growser Tor Pluggable Transports (macOS)";
constexpr const char kTorPluggableTransportComponentId[] =
    "kimmbaggbekmbachcdhfefjkkkibhfda";
constexpr const char kComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAujNFvBV5CHNM2qpHZlb9"
    "3HTpbFLnNxkdC6//qqQKqqL0tQUiGzGpLY3qdhci/1g/uNvR3tzM/Njj7aoSiSyA"
    "Z1eQxSI6MT7LMZFhOjpHNCBOpzCDh/3/ntBjajQj5nyqma04sdwhAIk5YQZiP9wu"
    "vnD0egL6eYuKQ2hmA3GLDxgMBxqpULi6og9Pwq2V4yY2HmukUJscpT09zmP2KjV0"
    "BXfL/S1Ec/xpuzPAty7pK8j+P5W8WqP+w5NxpjghR+ldnF1CUn3larksNhci76ej"
    "cbYMQFyf0j7n9ccb1SSP6x7c8F0DqZecRbb4JEP8f9Qw5ITJNGBJtRifWjSBMS4n"
    "0wIDAQAB";
#elif BUILDFLAG(IS_LINUX)
constexpr const char kComponentName[] = "Brave Pluggable Transports (Linux)";
constexpr const char kTorPluggableTransportComponentId[] =
    "apfggiafobakjahnkchiecbomjgigkkn";
constexpr const char kComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA6V9OyRC0zja5KfQ+"
    "cTbu1fgwD04dhcH36wU0NKlaERMSm/"
    "kZqYFFyxr3THAki6Ajo+X4m89EW0mIcjhgvOeUqyb1AzoVLwX/"
    "fKAM1Bf1q9zIjeDspaorSaniTtMMKcfxVI/"
    "e+xKsPc+95NtVsxEtU1PoQdKbBvQfSXkz3QJA3Z5/"
    "7vM+1omqwg5rCqvmqTXpyuhWOZl5lNeLRJ6zMfNiL/"
    "rkvq+A7h3DRhABQdjKrd+UfsPhQuMlVS3tCvoHNvB/"
    "qHEhWJqZzb0qpaMnHBCjZXD0s5PR5NxkEw/"
    "Yd2Xcxt1xdKULx0AZWD8wx5X2Idhy5rJAHiWQ5iZCdo1IHuAy4wIDAQAB";
#endif

constexpr const char kSnowflakeExecutableName[] = "tor-snowflake-brave";
constexpr const char kObfs4ExecutableName[] = "tor-obfs4-brave";

bool Initialize(const base::FilePath& install_dir) {
  const auto executables = {install_dir.AppendASCII(kSnowflakeExecutableName),
                            install_dir.AppendASCII(kObfs4ExecutableName)};

  for (const auto& executable : executables) {
    if (!base::PathExists(executable)) {
      LOG(ERROR) << executable << " doesn't exist";
      return false;
    }
#if BUILDFLAG(IS_POSIX)
    if (!base::SetPosixFilePermissions(executable, 0755)) {
      LOG(ERROR) << "Failed to set executable permission on " << executable;
      return false;
    }
#endif
  }
  return true;
}

BraveTorPluggableTransportUpdater::BraveTorPluggableTransportUpdater(
    BraveComponent::Delegate* component_delegate,
    PrefService* local_state,
    const base::FilePath& user_data_dir)
    : BraveComponent(component_delegate),
      local_state_(local_state),
      user_data_dir_(user_data_dir) {
  DCHECK(local_state);
}

BraveTorPluggableTransportUpdater::~BraveTorPluggableTransportUpdater() =
    default;

void BraveTorPluggableTransportUpdater::Register() {
  if (registered_)
    return;

  BraveComponent::Register(kComponentName, kTorPluggableTransportComponentId,
                           kComponentBase64PublicKey);
  registered_ = true;
  is_ready_ = false;
}

void BraveTorPluggableTransportUpdater::Unregister() {
  registered_ = false;
  is_ready_ = false;
}

void BraveTorPluggableTransportUpdater::Cleanup() {
  const base::FilePath component_dir =
      user_data_dir_.AppendASCII(kTorPluggableTransportComponentId);
  GetTaskRunner()->PostTask(
      FROM_HERE, base::GetDeletePathRecursivelyCallback(component_dir));
}

bool BraveTorPluggableTransportUpdater::IsReady() const {
  return is_ready_;
}

const base::FilePath&
BraveTorPluggableTransportUpdater::GetSnowflakeExecutable() const {
  return snowflake_path_;
}

const base::FilePath& BraveTorPluggableTransportUpdater::GetObfs4Executable()
    const {
  return obfs4_path_;
}

void BraveTorPluggableTransportUpdater::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveTorPluggableTransportUpdater::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveTorPluggableTransportUpdater::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&Initialize, install_dir),
      base::BindOnce(&BraveTorPluggableTransportUpdater::OnInitialized,
                     weak_ptr_factory_.GetWeakPtr(), install_dir));
}

void BraveTorPluggableTransportUpdater::OnInitialized(
    const base::FilePath& install_dir,
    bool success) {
  if (success) {
    // <component_id>/<version>
    const auto relative_component_path =
        base::FilePath::FromASCII(kTorPluggableTransportComponentId)
            .Append(install_dir.BaseName());

    snowflake_path_ =
        relative_component_path.AppendASCII(kSnowflakeExecutableName);
    obfs4_path_ = relative_component_path.AppendASCII(kObfs4ExecutableName);
  } else {
    snowflake_path_.clear();
    obfs4_path_.clear();
  }

  is_ready_ = success;

  for (auto& observer : observers_) {
    observer.OnPluggableTransportReady(success);
  }
}

}  // namespace tor
