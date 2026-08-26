/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/board_hosts.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/history/top_sites_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/new_tab_page/prefs/ntp_pref_names.h"
#include "components/ntp_tiles/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace brave_new_tab {

namespace {

const char kUserDataKey[] = "growser_board_hosts";

std::string HostOf(const GURL& url) {
  // GURL::host() hands back a view into the URL, so this has to copy.
  return url.SchemeIsHTTPOrHTTPS() ? std::string(url.host()) : std::string();
}

}  // namespace

base::flat_set<std::string> HostsFromCustomLinks(const base::ListValue& links,
                                                 size_t max) {
  base::flat_set<std::string> hosts;
  for (const base::Value& link : links) {
    if (hosts.size() >= max) {
      break;
    }
    if (!link.is_dict()) {
      continue;
    }
    const std::string* url = link.GetDict().FindString("url");
    if (!url) {
      continue;
    }
    // The cut is by host, not by entry: two links to one site are one tile's
    // worth of icon, and counting them twice would cut the list short.
    if (std::string host = HostOf(GURL(*url)); !host.empty()) {
      hosts.insert(std::move(host));
    }
  }
  return hosts;
}

base::flat_set<std::string> HostsFromMostVisited(
    const history::MostVisitedURLList& urls,
    size_t max) {
  base::flat_set<std::string> hosts;
  for (const history::MostVisitedURL& entry : urls) {
    if (hosts.size() >= max) {
      break;
    }
    if (std::string host = HostOf(entry.url); !host.empty()) {
      hosts.insert(std::move(host));
    }
  }
  return hosts;
}

BoardHosts::BoardHosts(Profile* profile)
    : profile_(profile),
      top_sites_(TopSitesFactory::GetForProfile(profile)) {
  if (top_sites_) {
    top_sites_->AddObserver(this);
  }
  pref_registrar_.Init(profile->GetPrefs());
  // The custom links list is the board whenever the user has arranged one.
  pref_registrar_.Add(
      ntp_tiles::prefs::kCustomLinksList,
      base::BindRepeating(&BoardHosts::Refresh, base::Unretained(this)));
  pref_registrar_.Add(
      ntp_tiles::prefs::kCustomLinksInitialized,
      base::BindRepeating(&BoardHosts::Refresh, base::Unretained(this)));
  // Which of the two lists the board is showing.
  pref_registrar_.Add(
      ntp_prefs::kNtpCustomLinksVisible,
      base::BindRepeating(&BoardHosts::Refresh, base::Unretained(this)));
  Refresh();
}

BoardHosts::~BoardHosts() {
  if (top_sites_) {
    top_sites_->RemoveObserver(this);
  }
}

// static
BoardHosts* BoardHosts::GetForProfile(Profile* profile) {
  if (!profile) {
    return nullptr;
  }
  auto* existing = static_cast<BoardHosts*>(profile->GetUserData(kUserDataKey));
  if (existing) {
    return existing;
  }
  if (!TopSitesFactory::GetForProfile(profile)) {
    // No top sites service means no board - an off-the-record profile, for
    // instance, which has no new tab board of its own.
    return nullptr;
  }
  auto tracker = base::WrapUnique(new BoardHosts(profile));
  auto* raw = tracker.get();
  profile->SetUserData(kUserDataKey, std::move(tracker));
  return raw;
}

// static
base::RepeatingCallback<bool(const GURL&)> BoardHosts::PolicyFor(
    Profile* profile) {
  BoardHosts* board = GetForProfile(profile);
  if (!board) {
    return base::RepeatingCallback<bool(const GURL&)>();
  }
  // Weak rather than raw: the tracker belongs to the profile and the driver
  // asking it belongs to a tab, and a tab can be torn down at a point where
  // the profile is already going away.
  return base::BindRepeating(
      [](base::WeakPtr<BoardHosts> board, const GURL& url) {
        return board && board->Contains(url);
      },
      board->weak_factory_.GetWeakPtr());
}

bool BoardHosts::Contains(const GURL& url) const {
  const std::string host = HostOf(url);
  return !host.empty() && hosts_.contains(host);
}

void BoardHosts::Refresh() {
  if (!top_sites_) {
    return;
  }
  top_sites_->GetMostVisitedURLs(base::BindOnce(
      &BoardHosts::OnMostVisitedURLs, weak_factory_.GetWeakPtr()));
}

void BoardHosts::OnMostVisitedURLs(const history::MostVisitedURLList& urls) {
  // One source, not both: the board shows the user's own list or the most
  // visited one, never a union of them. Collecting for the list that is not
  // on screen would be collecting for sites the board does not show, which is
  // the whole thing this class exists to avoid. Switching the mode fires the
  // pref observer, and the other list is collected from then on.
  hosts_ =
      profile_->GetPrefs()->GetBoolean(ntp_prefs::kNtpCustomLinksVisible)
          ? HostsFromCustomLinks(
                profile_->GetPrefs()->GetList(
                    ntp_tiles::prefs::kCustomLinksList),
                kMaxBoardSites)
          : HostsFromMostVisited(urls, kMaxBoardSites);
}

void BoardHosts::TopSitesLoaded(history::TopSites* top_sites) {
  Refresh();
}

void BoardHosts::TopSitesChanged(history::TopSites* top_sites,
                                 ChangeReason change_reason) {
  Refresh();
}

}  // namespace brave_new_tab
