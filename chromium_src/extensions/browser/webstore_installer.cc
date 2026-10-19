/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// growser#366: a Web Store install fetches its package from the Web Store.
// GetWebstoreUpdateUrl() is our url-source, and its `response=redirect` GET is
// answered only by Google's update2/crx, so every install got a 404. Proxying
// it hides nothing: the person is on Google's page already. Update checks of
// installed extensions still go through the url-source.

#include "extensions/common/extension_urls.h"

#define GetWebstoreUpdateUrl GetDefaultWebstoreUpdateUrl
#include <extensions/browser/webstore_installer.cc>
#undef GetWebstoreUpdateUrl
