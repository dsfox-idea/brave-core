/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/sidebar/sidebar_pinned_tabs_layout.h"

namespace sidebar {

int CalculatePinnedTabsCapacity(int available_height,
                                int entry_height,
                                int spacing,
                                int leading_height) {
  if (entry_height <= 0) {
    return 0;
  }

  // n entries need leading_height + spacing + n * (entry_height + spacing).
  const int height_for_entries = available_height - leading_height - spacing;
  if (height_for_entries <= 0) {
    return 0;
  }

  return height_for_entries / (entry_height + spacing);
}

}  // namespace sidebar
