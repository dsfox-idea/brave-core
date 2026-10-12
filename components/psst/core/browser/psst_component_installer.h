// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_

#include <cstdint>
#include <iterator>

#include "crypto/sha2.h"

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace psst {

// Growser-259: our own component, not Brave's. Brave's is signed with a key we
// do not have and served by go-updater.brave.com, which refuses a fork, so
// nothing ever installed. The hash below is SHA-256 of our key
// (../growser-keys/psst_component.pem), and the id is its first 128 bits:
// fgoclmlocoidkfpoifdeeihjpgnmbgno. scripts/make-psst-component.py builds the
// package from brave/psst-component and deploy/growser-backend serves it.
inline constexpr char kPsstComponentName[] =
    "Growser Privacy Settings Selection for Sites Tool (PSST) Files";
inline constexpr uint8_t kPsstComponentPublicKeySHA256[32] = {
    0x56, 0xe2, 0xbc, 0xbe, 0x2e, 0x83, 0xa5, 0xfe, 0x85, 0x34, 0x48,
    0x79, 0xf6, 0xdc, 0x16, 0xde, 0xf2, 0x49, 0xe3, 0x0e, 0x8f, 0x84,
    0x47, 0x6d, 0x6c, 0xf2, 0x14, 0xc2, 0x8d, 0x1a, 0xcf, 0x40};
static_assert(std::size(kPsstComponentPublicKeySHA256) ==
              crypto::kSHA256Length);

// Registers the PSST component with the component updater.
void RegisterPsstComponent(component_updater::ComponentUpdateService* cus);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_
