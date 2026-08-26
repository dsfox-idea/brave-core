/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ui/views/controls/highlight_path_generator.h"

// growser: the bookmark-import promo/nudge is out of the build (#22). There
// used to be a Brave override here injecting the "Import bookmarks"
// instruction layout into upstream bookmark_bar_view.cc through the
// BRAVE_LAYOUT macro and a BookmarkBarInstructionsView, created whenever the
// bar was empty. The class is gone and BRAVE_LAYOUT is empty, so there is no
// nagging hint. Bookmark import as a menu item is untouched.
#define BRAVE_LAYOUT
#include <chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc>
#undef BRAVE_LAYOUT