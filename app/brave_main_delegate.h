/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
#define BRAVE_APP_BRAVE_MAIN_DELEGATE_H_

#include <optional>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "build/build_config.h"
#include "chrome/app/chrome_main_delegate.h"

class BraveMainDelegateUnitTest;

#if BUILDFLAG(IS_MAC)
// growser (#206): our user data used to live inside Brave's own directory.
// These two carry it across to ours on the first start after the rename; the
// reasoning is in brave_main_delegate.cc.
namespace growser {

// The directory this build wrote to before #206, given the new-style |target|.
// Empty when |target| is not one of ours, because a guess here would be a
// guess about somebody else's data.
base::FilePath LegacyUserDataDirFor(const base::FilePath& target);

// Copies |legacy| to |target|, once, and only when |target| does not exist.
// Never moves and never deletes: |legacy| is Brave's directory as much as it
// is ours. Returns true only when this call created |target|.
bool MigrateUserDataDir(const base::FilePath& legacy,
                        const base::FilePath& target);

}  // namespace growser
#endif  // BUILDFLAG(IS_MAC)

// Chrome implementation of ContentMainDelegate.
class BraveMainDelegate : public ChromeMainDelegate {
 public:
  BraveMainDelegate(const BraveMainDelegate&) = delete;
  BraveMainDelegate& operator=(const BraveMainDelegate&) = delete;
#if BUILDFLAG(IS_ANDROID)
  BraveMainDelegate();
#endif

  // `timestamps.exe_entry_point_ticks` is the time at which the main function
  // of the executable was entered. On Windows, StartupTimestamps contains
  // timing information for calls to base::PreReadFile. `timestamps`' lifetime
  // does not need to last beyond the constructor call.
  explicit BraveMainDelegate(const StartupTimestamps& timestamps);
  ~BraveMainDelegate() override;

 protected:
  // content::ContentMainDelegate implementation:
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;
  std::optional<int> BasicStartupComplete() override;
  void PreSandboxStartup() override;
  std::optional<int> PostEarlyInitialization(
      ChromeMainDelegate::InvokedIn invoked_in) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveMainDelegateUnitTest,
                           DefaultCommandLineOverrides);
  FRIEND_TEST_ALL_PREFIXES(BraveMainDelegateUnitTest,
                           OverrideSwitchFromCommandLine);
  // growser (#79): the gate that sync stays off until we run a sync server.
  FRIEND_TEST_ALL_PREFIXES(BraveMainDelegateUnitTest, SyncIsDisabled);

  static void AppendCommandLineOptions();
};

#endif  // BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
