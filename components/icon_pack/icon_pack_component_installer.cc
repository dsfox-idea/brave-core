/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/icon_pack/icon_pack_component_installer.h"

#include <array>
#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/version.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"

namespace icon_pack {

namespace {

constexpr char kComponentName[] = "Growser Icon Pack";

// The id the browser insists on, and the SHA-256 of the key it is derived
// from. Both come from ../growser-keys/icon_pack_component.pem and nothing
// else: the id is the first 128 bits of that hash mapped onto a-p, so these
// two constants cannot disagree without the updater refusing whatever it
// downloads. The package is built by scripts/make-icon-pack-component.py and
// served by our own Worker (deploy/growser-backend) - Brave's component server
// answers a fork 403 and always has (growser#157).
constexpr char kComponentId[] = "binlmlgodhdmjjmjanaambhjpmfiakpo";

constexpr std::array<uint8_t, 32> kPublicKeySHA256 =
    {0x18, 0xdb, 0xcb, 0x6e, 0x37, 0x3c, 0x99, 0xc9, 0x0d, 0x00, 0xc1,
     0x79, 0xfc, 0x58, 0x0a, 0xfe, 0x93, 0x28, 0x57, 0x93, 0x7d, 0x2a,
     0x9e, 0x2d, 0x28, 0x04, 0xd0, 0xa5, 0x89, 0x0e, 0x0b, 0x87};

// The one file the package carries, written by build-icon-pack.py --emit-json.
constexpr char kPackFileName[] = "icon_pack.json";

base::FilePath& InstalledPath() {
  static base::NoDestructor<base::FilePath> path;
  return *path;
}

}  // namespace

IconPackComponentInstallerPolicy::IconPackComponentInstallerPolicy() = default;
IconPackComponentInstallerPolicy::~IconPackComponentInstallerPolicy() = default;

bool IconPackComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool IconPackComponentInstallerPolicy::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result
IconPackComponentInstallerPolicy::OnCustomInstall(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);
}

void IconPackComponentInstallerPolicy::OnCustomUninstall() {}

bool IconPackComponentInstallerPolicy::VerifyInstallation(
    const base::DictValue& manifest,
    const base::FilePath& install_dir) const {
  // A package without the pack in it is not an icon pack. Refusing here means
  // the browser keeps the one it has rather than recording an install that
  // hands over nothing.
  return base::PathExists(install_dir.AppendASCII(kPackFileName));
}

void IconPackComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    base::DictValue manifest) {
  VLOG(1) << "icon pack " << version.GetString() << " in "
          << install_dir.value();
  // Only the path is kept. Whether the file reads, parses, and is a format
  // this browser knows is decided where it is read - one place to be wrong
  // rather than two, and every one of those answers ends at the bundled pack.
  InstalledPath() = install_dir.AppendASCII(kPackFileName);
}

base::FilePath IconPackComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath::FromUTF8Unsafe(kComponentId);
}

void IconPackComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256.begin(), kPublicKeySHA256.end());
}

std::string IconPackComponentInstallerPolicy::GetName() const {
  return kComponentName;
}

update_client::InstallerAttributes
IconPackComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

bool IconPackComponentInstallerPolicy::IsBraveComponent() const {
  return true;
}

void RegisterIconPackComponent(component_updater::ComponentUpdateService* cus) {
  if (!cus) {
    return;
  }
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<IconPackComponentInstallerPolicy>());
  installer->Register(cus, base::BindOnce([]() {
                        brave_component_updater::BraveOnDemandUpdater::
                            GetInstance()
                                ->EnsureInstalled(kComponentId);
                      }));
}

const base::FilePath& InstalledIconPackPath() {
  return InstalledPath();
}

}  // namespace icon_pack
