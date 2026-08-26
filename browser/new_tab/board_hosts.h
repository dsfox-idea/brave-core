/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NEW_TAB_BOARD_HOSTS_H_
#define BRAVE_BROWSER_NEW_TAB_BOARD_HOSTS_H_

#include <string>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "base/values.h"
#include "components/history/core/browser/top_sites.h"
#include "components/history/core/browser/top_sites_observer.h"
#include "components/prefs/pref_change_registrar.h"

class GURL;
class Profile;

namespace brave_new_tab {

// The board holds this many tiles at most, so a site further down the most
// visited list is not on it and its large icon would be collected for nothing.
inline constexpr size_t kMaxBoardSites = 16;

// Declared here so the selection can be tested without a profile: which hosts
// count, in what order, and where the list is cut.
base::flat_set<std::string> HostsFromCustomLinks(const base::ListValue& links,
                                                 size_t max);
base::flat_set<std::string> HostsFromMostVisited(
    const history::MostVisitedURLList& urls,
    size_t max);

// The hosts the new tab board shows - the custom links the user arranged, or
// the most visited sites when they have not.
//
// It exists so that a page being visited can be asked, in constant time,
// whether it is one of those. Only the board draws an icon large enough to
// need more than the 16 dip favicon, so only these sites should pay the extra
// request for one (growser#90). Everything else keeps upstream's behaviour
// exactly.
//
// Kept on the Profile rather than as a KeyedService: it holds a set of strings
// and two observers, and a factory around that would be more machinery than
// the thing itself.
class BoardHosts : public base::SupportsUserData::Data,
                   public history::TopSitesObserver {
 public:
  ~BoardHosts() override;

  BoardHosts(const BoardHosts&) = delete;
  BoardHosts& operator=(const BoardHosts&) = delete;

  // Creates the tracker on first use. Null for a profile with no top sites
  // service, which is what an off-the-record profile has.
  static BoardHosts* GetForProfile(Profile* profile);

  // True when `url`'s host is one the board shows. Safe to call for any URL,
  // including ones that are not http(s).
  bool Contains(const GURL& url) const;

  // The policy to hand FaviconDriverImpl::SetLargeIconPolicy. Empty for a
  // profile with no board, which the driver reads as "collect nothing" - so
  // an off-the-record window behaves exactly as upstream's.
  static base::RepeatingCallback<bool(const GURL&)> PolicyFor(Profile* profile);

 private:
  explicit BoardHosts(Profile* profile);

  void Refresh();
  void OnMostVisitedURLs(const history::MostVisitedURLList& urls);

  // history::TopSitesObserver:
  void TopSitesLoaded(history::TopSites* top_sites) override;
  void TopSitesChanged(history::TopSites* top_sites,
                       ChangeReason change_reason) override;

  raw_ptr<Profile> profile_ = nullptr;
  scoped_refptr<history::TopSites> top_sites_;
  PrefChangeRegistrar pref_registrar_;
  base::flat_set<std::string> hosts_;
  base::WeakPtrFactory<BoardHosts> weak_factory_{this};
};

}  // namespace brave_new_tab

#endif  // BRAVE_BROWSER_NEW_TAB_BOARD_HOSTS_H_
