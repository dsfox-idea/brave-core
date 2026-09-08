/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ICON_PACK_ICON_PACK_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_ICON_PACK_ICON_PACK_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/values.h"
#include "components/component_updater/component_installer.h"
#include "components/update_client/update_client.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace icon_pack {

// growser#190: the new tab board's icon pack, delivered as a component.
//
// The page carries a pack in its own bundle and that copy is the FLOOR - this
// only ever hands over something newer. So every failure here is quiet by
// design: a component that never arrives, a directory without the file, a
// machine that never reaches us, all end at the board the browser shipped
// with.
//
// The browser fetches the WHOLE pack on the updater's schedule, which is what
// keeps the property bundling was chosen for: the request says nothing about
// which sites are on anyone's board (growser#96). A per-site lookup would
// undo that, and a privileged page may not make one anyway.
class IconPackComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
 public:
  IconPackComponentInstallerPolicy();
  ~IconPackComponentInstallerPolicy() override;

  IconPackComponentInstallerPolicy(const IconPackComponentInstallerPolicy&) =
      delete;
  IconPackComponentInstallerPolicy& operator=(
      const IconPackComponentInstallerPolicy&) = delete;

  // component_updater::ComponentInstallerPolicy
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
      const base::DictValue& manifest,
      const base::FilePath& install_dir) override;
  void OnCustomUninstall() override;
  bool VerifyInstallation(const base::DictValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::DictValue manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;
  bool IsBraveComponent() const override;
};

// Registers the icon pack component with the component updater.
void RegisterIconPackComponent(component_updater::ComponentUpdateService* cus);

// The installed pack, or an empty path when no component has arrived - which
// is every fresh profile, and not a failure. The file is deliberately not read
// here: reading blocks, and the one caller already has a thread for it.
const base::FilePath& InstalledIconPackPath();

}  // namespace icon_pack

#endif  // BRAVE_COMPONENTS_ICON_PACK_ICON_PACK_COMPONENT_INSTALLER_H_
