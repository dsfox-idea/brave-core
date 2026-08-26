/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/grit/brave_components_strings.h"

// growser (#95): say why a Russian-rooted certificate was refused.
//
// We trust the Russian national CA for a fixed list of domains (#36). A site
// on that root but outside the list therefore fails with the generic
// "your connection is not private", which tells the user nothing and tells us
// nothing either - the list is a snapshot and will always be missing someone.
// One sentence turns a mystery into a decision, and gives the user a reason to
// report the gap.
//
// The issuer common name is enough to recognise the chain: the intermediate is
// "Russian Trusted Sub CA", the root "Russian Trusted Root CA". Matching the
// prefix keeps working if they add another intermediate.
#define BRAVE_SSL_ERROR_INFO_AUTHORITY_INVALID                              \
  if (cert &&                                                               \
      cert->issuer().common_name.starts_with("Russian Trusted")) {          \
    details = base::StrCat(                                                 \
        {details, u"\n\n",                                            \
         l10n_util::GetStringUTF16(                                         \
             IDS_GROWSER_CERT_ERROR_RU_TRUST_NOT_LISTED)});                 \
  }

#include <components/ssl_errors/error_info.cc>

#undef BRAVE_SSL_ERROR_INFO_AUTHORITY_INVALID
