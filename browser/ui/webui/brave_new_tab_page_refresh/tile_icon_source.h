/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_TILE_ICON_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_TILE_ICON_SOURCE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon_base/favicon_types.h"
#include "content/public/browser/url_data_source.h"

class GURL;
class Profile;

namespace brave_new_tab_page_refresh {

// Serves the largest icon a site has given us, at the resolution it was stored
// in - chrome://growser-tile-icon/?pageUrl=<url>.
//
// chrome://favicon2 cannot do this. It refuses a size of 0 (its parser demands
// a positive integer) and resizes whatever it finds up to the size asked for,
// so a 32 pixel favicon comes back as a blurred 64 and the page cannot even
// tell. On a new tab tile that is the difference between a logo and a smudge.
//
// This source asks for no particular size, prefers the touch and web manifest
// icons - which are the large ones, 180 to 512 pixels - and falls back to the
// ordinary favicon. Nothing is ever scaled up: a site that only offers 32
// pixels is drawn at 32 pixels, which is honest and sharp rather than large
// and soft.
class TileIconSource : public content::URLDataSource {
 public:
  static constexpr char kHost[] = "growser-tile-icon";

  explicit TileIconSource(Profile* profile);
  ~TileIconSource() override;

  TileIconSource(const TileIconSource&) = delete;
  TileIconSource& operator=(const TileIconSource&) = delete;

  // content::URLDataSource:
  std::string GetSource() override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;
  void StartDataRequest(
      const GURL& url,
      const content::WebContents::Getter& wc_getter,
      content::URLDataSource::GotDataCallback callback) override;

 private:
  void OnIconAvailable(GURL page_url,
                       content::URLDataSource::GotDataCallback callback,
                       const favicon_base::FaviconRawBitmapResult& result);

  raw_ptr<Profile> profile_ = nullptr;
  base::CancelableTaskTracker tracker_;
  base::WeakPtrFactory<TileIconSource> weak_factory_{this};
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_TILE_ICON_SOURCE_H_
