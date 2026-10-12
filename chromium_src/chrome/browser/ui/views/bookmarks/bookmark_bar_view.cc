/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ui/views/controls/highlight_path_generator.h"

class BrowserWindowInterface;

// growser: the bookmark-import promo/nudge is out of the build (#22). Brave
// lays out an "Import bookmarks" BookmarkBarInstructionsView whenever the bar
// is empty; that class is gone here, so there is no nagging hint. Bookmark
// import as a menu item is untouched.
// Growser-350: upstream's recipe
// (rewrite/chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc.yaml) calls
// LayoutBookmarkBarInstructionsView() from BookmarkBarView::Layout directly
// now, where it used to go through the BRAVE_LAYOUT macro. The function stays
// and lays out nothing.
namespace {
void LayoutBookmarkBarInstructionsView(views::View*,
                                       bookmarks::BookmarkModel*,
                                       BrowserWindowInterface*,
                                       int,
                                       int,
                                       int,
                                       int) {}
}  // namespace

#include <chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc>
