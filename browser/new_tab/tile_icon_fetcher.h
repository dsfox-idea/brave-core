/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_TILE_ICON_FETCHER_H_
#define BRAVE_BROWSER_NEW_TAB_TILE_ICON_FETCHER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/time/time.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;
class Profile;
class SkBitmap;

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace brave_new_tab {

// Fetches a large icon for a board site the browser has no large icon for.
//
// Icons are normally collected while a site is being visited, which covers a
// site the user goes to. It does not cover the case the owner asked about: a
// site rises onto the board and is not visited again for a while, so the board
// draws whatever small favicon it happens to hold. This closes that one gap
// with a single request, and only that gap.
//
// Two conventions, and nothing cleverer: Safari has probed
// /apple-touch-icon.png at the site root since 2008, so most sites serve one
// even when they never declare it in their markup - apple.com and github.com
// both do, at 152 and 120 pixels, having declared nothing. A site that serves
// none costs one 404 and is not asked again for three weeks.
//
// What this deliberately is not: a crawl. One URL, one origin - the site the
// user already has on their board - no cookies, no referrer, and at most one
// attempt per host per `kRetryInterval` whether it found anything or not.
class TileIconFetcher : public base::SupportsUserData::Data {
 public:
  // How long an attempt stands, successful or not. Without the "or not" a site
  // that serves no such icon would be asked again on every new tab.
  static constexpr base::TimeDelta kRetryInterval = base::Days(21);

  // Below this the board would have to stretch the icon on a 2x display, which
  // is the whole complaint this exists to answer.
  static constexpr int kWantedIconSize = 112;

  ~TileIconFetcher() override;

  TileIconFetcher(const TileIconFetcher&) = delete;
  TileIconFetcher& operator=(const TileIconFetcher&) = delete;

  static TileIconFetcher* GetForProfile(Profile* profile);
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Asks for a large icon for `page_url` unless one was asked for recently.
  // Returns at once; anything found lands in the favicon database, which is
  // where the next new tab reads from.
  void MaybeFetch(const GURL& page_url);

 private:
  explicit TileIconFetcher(Profile* profile);

  bool ShouldTry(const std::string& host) const;
  void RecordAttempt(const std::string& host);

  void OnResponse(GURL page_url,
                  GURL icon_url,
                  std::optional<std::string> body);
  void OnDecoded(GURL page_url,
                 GURL icon_url,
                 std::unique_ptr<std::string> body,
                 const SkBitmap& bitmap);

  void StartNext();

  raw_ptr<Profile> profile_ = nullptr;

  // One request in flight, the rest waiting. The board asks for every tile at
  // once and a burst of requests is exactly what this must not be; a queue
  // keeps that promise while still finishing the board in one sitting rather
  // than one site per new tab.
  std::vector<GURL> pending_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  base::WeakPtrFactory<TileIconFetcher> weak_factory_{this};
};

// Exposed for testing: the URL a host is asked for, and whether an attempt
// recorded at `last` is old enough to repeat at `now`.
GURL TouchIconURLFor(const GURL& page_url);
bool AttemptHasLapsed(base::Time last, base::Time now);

}  // namespace brave_new_tab

#endif  // BRAVE_BROWSER_NEW_TAB_TILE_ICON_FETCHER_H_
