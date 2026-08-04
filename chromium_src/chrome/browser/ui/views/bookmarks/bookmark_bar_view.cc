/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/check_op.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "ui/views/controls/highlight_path_generator.h"

// growser: промо/nudge импорта закладок выпилен из билда (#22). Раньше здесь
// был Brave-override, инжектирующий layout «Import bookmarks»-инструкций в
// upstream bookmark_bar_view.cc через макрос BRAVE_LAYOUT + класс
// BookmarkBarInstructionsView (создавался на пустой панели). Класс удалён,
// BRAVE_LAYOUT — empty (no-op): навязчивой подсказки нет. Сам импорт закладок
// как функцию меню не трогаем.
#define BRAVE_LAYOUT
#include <chrome/browser/ui/views/bookmarks/bookmark_bar_view.cc>
#undef BRAVE_LAYOUT