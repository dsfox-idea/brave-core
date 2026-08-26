/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/tile_icon_source.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/new_tab/tile_icon_fetcher.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_new_tab_page_refresh {

namespace {

// Below this a large icon is not worth preferring over the ordinary favicon,
// and the tile is better off with whichever is bigger. A tile draws its icon
// at 56 pixels, so this is the point where scaling up would begin.
constexpr int kMinimumLargeIconSize = 56;

}  // namespace

TileIconSource::TileIconSource(Profile* profile) : profile_(profile) {}

TileIconSource::~TileIconSource() = default;

std::string TileIconSource::GetSource() {
  return kHost;
}

std::string TileIconSource::GetMimeType(const GURL& url) {
  // The favicon database stores every bitmap PNG-encoded, whatever the site
  // originally served.
  return "image/png";
}

bool TileIconSource::AllowCaching() {
  // A site can change its icon, and the board is drawn from a local database
  // read - cheap enough that serving a stale icon would be the only cost.
  return false;
}

void TileIconSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    content::URLDataSource::GotDataCallback callback) {
  std::string page_url_string;
  if (!net::GetValueForKeyInQuery(url, "pageUrl", &page_url_string)) {
    std::move(callback).Run(nullptr);
    return;
  }

  GURL page_url(page_url_string);
  if (!page_url.is_valid()) {
    std::move(callback).Run(nullptr);
    return;
  }

  favicon::FaviconService* favicon_service =
      FaviconServiceFactory::GetForProfile(profile_,
                                           ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Ordered by preference: the large icons a page declares for home screens
  // and manifests first, the ordinary favicon second. If neither has anything
  // above the threshold, the largest of all of them comes back - still at its
  // own resolution.
  favicon_service->GetLargestRawFaviconForPageURL(
      page_url,
      {{favicon_base::IconType::kWebManifestIcon,
        favicon_base::IconType::kTouchIcon,
        favicon_base::IconType::kTouchPrecomposedIcon},
       {favicon_base::IconType::kFavicon}},
      kMinimumLargeIconSize,
      base::BindOnce(&TileIconSource::OnIconAvailable,
                     weak_factory_.GetWeakPtr(), page_url, std::move(callback)),
      &tracker_);
}

void TileIconSource::OnIconAvailable(
    GURL page_url,
    content::URLDataSource::GotDataCallback callback,
    const favicon_base::FaviconRawBitmapResult& result) {
  // The board is drawn from what we have now; if that is too small, ask the
  // site for its large icon so the next new tab is better. This is the one
  // case the visit-time collection cannot cover: a site rose onto the board
  // and has not been visited since (growser#90).
  if (!result.is_valid() ||
      result.pixel_size.width() <
          brave_new_tab::TileIconFetcher::kWantedIconSize) {
    if (auto* fetcher =
            brave_new_tab::TileIconFetcher::GetForProfile(profile_)) {
      fetcher->MaybeFetch(page_url);
    }
  }

  // An empty response rather than a placeholder: the page knows what to draw
  // for a site whose icon has not arrived, and a placeholder here would be
  // indistinguishable from a real icon to it.
  std::move(callback).Run(result.is_valid() ? result.bitmap_data : nullptr);
}

}  // namespace brave_new_tab_page_refresh
