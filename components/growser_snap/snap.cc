/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/growser_snap/snap.h"

#include "base/environment.h"
#include "sandbox/policy/switches.h"

namespace growser {

bool IsRunningInSnap(base::Environment* env) {
  auto value = env->GetVar("GROWSER_SNAP");
  return value && *value == "1";
}

bool IsSnapPackagingFlag(std::string_view flag) {
  if (flag != sandbox::policy::switches::kNoSandbox) {
    return false;
  }
  auto env = base::Environment::Create();
  return IsRunningInSnap(env.get());
}

}  // namespace growser
