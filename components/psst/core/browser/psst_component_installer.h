// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_

namespace component_updater {
class ComponentUpdateService;
}  // namespace component_updater

namespace psst {

// Growser-259: our own component, not Brave's. Brave's is signed with a key we
// do not have and served by go-updater.brave.com, which refuses a fork, so
// nothing ever installed. The id is the first 128 bits of SHA-256 of the key
// below (../growser-keys/psst_component.pem); scripts/make-psst-component.py
// builds the package from brave/psst-component and deploy/growser-backend
// serves it. The three constants move together: the updater registers the id
// it derives from the key, not this string.
inline constexpr char kPsstComponentName[] =
    "Growser Privacy Settings Selection for Sites Tool (PSST) Files";
inline constexpr char kPsstComponentId[] = "fgoclmlocoidkfpoifdeeihjpgnmbgno";
inline constexpr char kPsstComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA7PV0Bdn/cSPYqnS8JsWLJsc3Du6lpDMJ"
    "ZVpa14hgYhHjd5I8DRUpYGQFrVQ+NfiyPRcxLRNMmzV82I4lUMNcwpWaP9A74VZSdqDLB4Co5TmS"
    "oKT/yn9qr9konjy7jcr6rkuYMQyO8Gthj4uAPZ7H+iEFy3BGAfAMFFrZXHVHI/y6pR+2wxkk36Qr"
    "9ZwPunRsDivSPFGSZQU5I+leNO9iYNXA6893KSALFT0lJVXj0kutj7CXrw3ucJp0TBRiRbqOmu7g"
    "QGQLotHKIXEEAxer/2iAzmy8oZait4amndP1czuFxYRgTToPygiH5GnNn9+U6DcsmWY8TLc4vRts"
    "PozELQIDAQAB";

// Registers the PSST component with the component updater.
void RegisterPsstComponent(component_updater::ComponentUpdateService* cus);

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_COMPONENT_INSTALLER_H_
