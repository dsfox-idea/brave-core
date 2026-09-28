/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/pref_service_builder_utils.h"

#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/common/bookmark_bar_visibility_state.h"  // Growser-28
#include "components/bookmarks/common/bookmark_pref_names.h"  // Growser-28
#include "components/signin/public/base/signin_pref_names.h"
#include "components/spellcheck/browser/pref_names.h"
#include "ui/color/system_theme.h"

#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl
#include <chrome/browser/profiles/pref_service_builder_utils.cc>
#undef RegisterProfilePrefs

// Prefs for KeyedService
void RegisterProfilePrefs(bool is_signin_profile,
                          const std::string& locale,
                          user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs_ChromiumImpl(is_signin_profile, locale, registry);

  // Change default pref values that are registered by keyed services

  // Disable spell check service
  registry->SetDefaultPrefValue(
      spellcheck::prefs::kSpellCheckUseSpellingService, base::Value(false));

  registry->SetDefaultPrefValue(prefs::kSigninAllowedOnNextStartup,
                                base::Value(false));
#if !BUILDFLAG(IS_ANDROID)
  // Growser-28: the bookmark bar is hidden everywhere by default, the new tab
  // page included - that row is what makes the toolbar look double height.
  // Since 155 Brave uses Chromium's visibility pref, whose default is
  // kOnlyShowOnNtp; ours lived on Brave's own pref, which is gone. The pref
  // is registered by BookmarkModelFactory, hence here and not in
  // brave_profile_prefs.cc, which runs before the keyed services register.
  registry->SetDefaultPrefValue(
      bookmarks::prefs::kBookmarkBarVisibilityState,
      base::Value(static_cast<int>(
          bookmarks::BookmarkBarVisibilityState::kAlwaysHide)));
#endif

#if BUILDFLAG(IS_LINUX)
  // Use brave theme by default instead of gtk theme.
  registry->SetDefaultPrefValue(
      prefs::kSystemTheme,
      base::Value(static_cast<int>(ui::SystemTheme::kDefault)));
#endif
}
