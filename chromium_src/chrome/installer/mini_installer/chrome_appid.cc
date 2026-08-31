/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/installer/mini_installer/appid.h"

namespace google_update {

#if defined(OFFICIAL_BUILD)
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Brave Origin uses separate app GUIDs from Brave Browser to allow
// side-by-side installation and independent update infrastructure.
const wchar_t kAppGuid[] = L"{F1EF32DE-F987-4289-81D2-6C4780027F9B}";
const wchar_t kBetaAppGuid[] = L"{56DA94FD-D872-416B-BFC4-1D7011DA7473}";
const wchar_t kDevAppGuid[] = L"{716D6A4A-D071-47A8-AC64-DBDE3EE3797B}";
const wchar_t kSxSAppGuid[] = L"{50474E96-9CD2-4BC8-B0A7-0D4B6EF2E709}";
#else
// growser (#128): our GUIDs, matching chromium_install_modes.h - stable is
// the developer-mode identity every existing install already registered.
const wchar_t kAppGuid[] = L"{B003E671-954C-4C60-A0D4-4172D74FD4C1}";
const wchar_t kBetaAppGuid[] = L"{3C158BB8-E5D2-4180-B58E-E3F9BB45F4F6}";
const wchar_t kDevAppGuid[] = L"{81B04A9D-2573-4FC0-BE7F-79583A591680}";
const wchar_t kSxSAppGuid[] = L"{97E119AD-4AD1-473F-AABA-5204810AA9DE}";
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#else
const wchar_t kAppGuid[] = L"";
#endif

}  // namespace google_update
