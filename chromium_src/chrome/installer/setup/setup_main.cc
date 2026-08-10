/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "brave/installer/setup/archive_patch_helper.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/setup/brand_behaviors.h"
#include "chrome/installer/setup/installer_state.h"
#include "chrome/installer/setup/modify_params.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/initial_preferences.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/installer_util_strings.h"
#include "chrome/installer/util/util_constants.h"

namespace {

// The code in this function used to be upstream and had to be restored in Brave
// to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
bool BraveHandleNonInstallCmdLineOptions(
    installer::ModifyParams& modify_params,
    const base::CommandLine& cmd_line,
    const installer::InitialPreferences& prefs,
    int* exit_code) {
  if (!cmd_line.HasSwitch(installer::switches::kUpdateSetupExe)) {
    return false;
  }
  installer::InstallerState* installer_state =
      &(*modify_params.installer_state);
  const base::FilePath& setup_exe = *modify_params.setup_path;

  installer_state->SetStage(installer::UPDATING_SETUP);
  installer::InstallStatus status = installer::SETUP_PATCH_FAILED;
  // If --update-setup-exe command line option is given, we apply the given
  // patch to current exe, and store the resulting binary in the path
  // specified by --new-setup-exe. But we need to first unpack the file
  // given in --update-setup-exe.

  const base::FilePath compressed_archive(
      cmd_line.GetSwitchValuePath(installer::switches::kUpdateSetupExe));
  VLOG(1) << "Opening archive " << compressed_archive.value();
  // The top unpack failure result with 28 days aggregation (>=0.01%)
  // Setup.Install.LzmaUnPackResult_SetupExePatch
  // 0.02% PATH_NOT_FOUND
  //
  // More information can also be found with metric:
  // Setup.Install.LzmaUnPackNTSTATUS_SetupExePatch

  // We use the `new_setup_exe` directory as the working directory for
  // `ArchivePatchHelper::UncompressAndPatch`. For System installs, this
  // directory would be under %ProgramFiles% (a directory that only admins can
  // write to by default) and hence a secure location.
  const base::FilePath new_setup_exe(
      cmd_line.GetSwitchValuePath(installer::switches::kNewSetupExe));
  if (installer::ArchivePatchHelper::UncompressAndPatch(
          new_setup_exe.DirName(), compressed_archive, setup_exe, new_setup_exe,
          installer::UnPackConsumer::UNCOMPRESSED_CHROME_ARCHIVE)) {
    status = installer::NEW_VERSION_UPDATED;
  }

  *exit_code = InstallUtil::GetInstallReturnCode(status);
  if (*exit_code) {
    LOG(WARNING) << "setup.exe patching failed.";
    installer_state->WriteInstallerResult(status, IDS_SETUP_PATCH_FAILED_BASE,
                                          nullptr);
  }
  return true;
}

constexpr char kBraveReferralCode[] = "brave-referral-code";

void SavePromoCode(installer::InstallStatus install_status) {
  if (!InstallUtil::GetInstallReturnCode(install_status)) {
    const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
    if (cmd_line.HasSwitch(kBraveReferralCode)) {
      const std::string referral_code =
          cmd_line.GetSwitchValueASCII(kBraveReferralCode);
      if (!referral_code.empty()) {
        base::FilePath user_data_dir;
        base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
        base::FilePath referral_code_path =
            user_data_dir.AppendASCII("promoCode");
        if (!base::WriteFile(referral_code_path, referral_code)) {
          LOG(ERROR) << "Failed to write referral code " << referral_code
                     << " to " << referral_code_path;
        }
      }
    }
  }
}

// growser (#51): install the updater alongside the browser.
//
// Chromium's setup.exe does not do this - in Google's product the metainstaller
// installs Omaha, and we have no metainstaller. updater.exe ships in the version
// directory (chrome.release) and installing it is one call: it copies itself
// into its own directory and registers a scheduled task. Measured: exit 0,
// %LOCALAPPDATA%\Growser\GrowserUpdater created, task registered.
//
// Deliberately not fatal. A browser that installed but whose updater did not is
// a browser that works and does not update; failing the whole installation over
// it would be the worse trade. It is also idempotent - running it again on an
// existing install is how an upgrade re-registers.
void InstallUpdater(installer::InstallStatus install_status) {
  if (InstallUtil::GetInstallReturnCode(install_status)) {
    return;  // The browser did not install; there is nothing to update.
  }
  base::FilePath setup_exe;
  if (!base::PathService::Get(base::FILE_EXE, &setup_exe)) {
    return;
  }
  // ...\Application\<version>\Installer\setup.exe -> ...\<version>\updater.exe
  const base::FilePath updater =
      setup_exe.DirName().DirName().Append(FILE_PATH_LITERAL("updater.exe"));
  if (!base::PathExists(updater)) {
    LOG(WARNING) << "No updater at " << updater << "; updates are off.";
    return;
  }
  base::CommandLine command(updater);
  command.AppendSwitch("install");
  command.AppendSwitch("silent");
  base::LaunchOptions options;
  options.start_hidden = true;
  if (!base::LaunchProcess(command, options).IsValid()) {
    LOG(WARNING) << "Could not start the updater installer.";
  }
}

}  // namespace

#define DoLegacyCleanups         \
  SavePromoCode(install_status); \
  InstallUpdater(install_status); \
  DoLegacyCleanups

// The macros BRAVE_SETUP_MAIN, UpdateInstallStatus and BRAVE_INSTALL_PRODUCTS
// contain code that used to be upstream and had to be restored in Brave to
// support delta updates on Windows until we are on Omaha 4. See
// github.com/brave/brave-core/pull/31937.
#define BRAVE_SETUP_MAIN                                                  \
  if (BraveHandleNonInstallCmdLineOptions(modify_params, cmd_line, prefs, \
                                          &exit_code)) {                  \
    return exit_code;                                                     \
  }
#define BRAVE_INSTALL_PRODUCTS UpdateInstallStatus();
#define UpdateInstallStatus() \
  UpdateInstallStatus(installer_state->archive_type, install_status)

#include <chrome/installer/setup/setup_main.cc>

#undef UpdateInstallStatus
#undef BRAVE_INSTALL_PRODUCTS
#undef BRAVE_SETUP_MAIN
#undef DoLegacyCleanups
