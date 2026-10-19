/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/installer/util/brave_shell_util.h"

#include <windows.h>  // Growser-365

#include <appmodel.h>  // Growser-365

#include "base/notreached.h"
#include "chrome/install_static/install_util.h"
#include "components/version_info/channel.h"

namespace installer {

std::wstring GetProgIdForFileType() {
  switch (install_static::GetChromeChannel()) {
    case version_info::Channel::STABLE:
      return L"BraveFile";
    case version_info::Channel::BETA:
      return L"BraveBFile";
    case version_info::Channel::DEV:
      return L"BraveDFile";
    case version_info::Channel::CANARY:
      return L"BraveSSFile";
    default:
      break;
  }
  // install_static::GetChromeChannel() only gives above four types
  // for official build. Brave does not support an installer built from an
  // unofficial build and hit NOTREACHED here.
  //
  // growser (#50): we do exactly that - GROWSER_NON_OFFICIAL means
  // OFFICIAL_BUILD is never defined, so the channel is none of the four and
  // this is reachable. It cost a real failure: uninstall crashed here after it
  // had already removed the registry entries and the Start menu shortcut but
  // before deleting any files, leaving 676 MB behind and no way to uninstall
  // again. The value matches the sole install mode we build, whose ProgID
  // prefixes are GrowserDHTM / GrowserDPDF.
  return L"GrowserDevFile";
}

bool ShouldUseFileTypeProgId(std::wstring_view ext) {
  return (ext == L".pdf" || ext == L".svg");
}

// Growser-365 (moved here from #132's copy in the notification bridge).
std::wstring GetPackageAppUserModelId() {
  UINT32 length = 0;
  if (::GetCurrentApplicationUserModelId(&length, nullptr) !=
      ERROR_INSUFFICIENT_BUFFER) {
    // APPMODEL_ERROR_NO_PACKAGE for every non-packaged build.
    return std::wstring();
  }
  std::wstring app_user_model_id(length, L'\0');
  if (::GetCurrentApplicationUserModelId(&length, app_user_model_id.data()) !=
      ERROR_SUCCESS) {
    return std::wstring();
  }
  // On success `length` counts the terminating null, which the string owns.
  app_user_model_id.resize(length ? length - 1 : 0);
  return app_user_model_id;
}

}  // namespace installer
