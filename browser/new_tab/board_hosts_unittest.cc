/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/board_hosts.h"

#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/history/core/browser/top_sites.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_new_tab {

namespace {

base::ListValue CustomLinks(const std::vector<std::string>& urls) {
  base::ListValue links;
  for (const std::string& url : urls) {
    base::DictValue link;
    link.Set("url", url);
    link.Set("title", "title");
    links.Append(std::move(link));
  }
  return links;
}

history::MostVisitedURLList MostVisited(const std::vector<std::string>& urls) {
  history::MostVisitedURLList list;
  for (const std::string& url : urls) {
    history::MostVisitedURL entry;
    entry.url = GURL(url);
    list.push_back(entry);
  }
  return list;
}

}  // namespace

TEST(BoardHostsTest, TakesTheHostOfEachLink) {
  const auto hosts = HostsFromCustomLinks(
      CustomLinks({"https://example.com/some/page", "http://other.test/"}),
      kMaxBoardSites);
  EXPECT_EQ(2u, hosts.size());
  EXPECT_TRUE(hosts.contains("example.com"));
  EXPECT_TRUE(hosts.contains("other.test"));
}

TEST(BoardHostsTest, SkipsWhatIsNotAWebPage) {
  // A tile can hold anything the user typed, and a file or a browser page has
  // no icon to fetch from a site.
  const auto hosts = HostsFromCustomLinks(
      CustomLinks({"file:///c:/notes.txt", "chrome://settings",
                   "not a url", "https://example.com/"}),
      kMaxBoardSites);
  EXPECT_EQ(1u, hosts.size());
  EXPECT_TRUE(hosts.contains("example.com"));
}

TEST(BoardHostsTest, SkipsMalformedEntries) {
  base::ListValue links = CustomLinks({"https://example.com/"});
  links.Append(base::Value("a bare string"));
  base::DictValue without_url;
  without_url.Set("title", "no url here");
  links.Append(std::move(without_url));

  const auto hosts = HostsFromCustomLinks(links, kMaxBoardSites);
  EXPECT_EQ(1u, hosts.size());
}

TEST(BoardHostsTest, CountsHostsRatherThanEntries) {
  // Two links into one site are one tile's worth of icon; counting them twice
  // would cut the list short of the sites that follow.
  const auto hosts = HostsFromCustomLinks(
      CustomLinks({"https://example.com/one", "https://example.com/two",
                   "https://other.test/"}),
      2);
  EXPECT_EQ(2u, hosts.size());
  EXPECT_TRUE(hosts.contains("other.test"));
}

TEST(BoardHostsTest, StopsAtTheBoardSize) {
  std::vector<std::string> urls;
  for (int i = 0; i < 40; ++i) {
    urls.push_back("https://site" + base::NumberToString(i) + ".test/");
  }
  EXPECT_EQ(kMaxBoardSites, HostsFromCustomLinks(CustomLinks(urls),
                                                 kMaxBoardSites).size());
  EXPECT_EQ(kMaxBoardSites,
            HostsFromMostVisited(MostVisited(urls), kMaxBoardSites).size());
}

TEST(BoardHostsTest, TakesTheMostVisitedInOrder) {
  // The cut has to keep the front of the list: those are the tiles the board
  // actually shows.
  const auto hosts = HostsFromMostVisited(
      MostVisited({"https://first.test/", "https://second.test/",
                   "https://third.test/"}),
      2);
  EXPECT_EQ(2u, hosts.size());
  EXPECT_TRUE(hosts.contains("first.test"));
  EXPECT_TRUE(hosts.contains("second.test"));
  EXPECT_FALSE(hosts.contains("third.test"));
}

TEST(BoardHostsTest, HandlesAnEmptyBoard) {
  EXPECT_TRUE(HostsFromCustomLinks(base::ListValue(), kMaxBoardSites).empty());
  EXPECT_TRUE(
      HostsFromMostVisited(history::MostVisitedURLList(), kMaxBoardSites)
          .empty());
}

}  // namespace brave_new_tab
