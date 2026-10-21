/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/strings/string_util.h"  // Growser-370
#include "components/safe_browsing/core/common/proto/csd.pb.h"
#include "url/gurl.h"  // Growser-370

// Growser-370: when the ping's destination is one the network layer cancels
// anyway (#329), finish where it would be sent, the same way upstream does
// when pings are off, instead of building it and resolving the stub host.
#define BRAVE_ON_REQUEST_BUILT_FILTER                                  \
  BraveFilterRequest(client_download_request_.get());                  \
  if (IsBlockedDownloadPingUrl(service()->GetDownloadRequestUrl())) {  \
    FinishRequest(DownloadCheckResult::UNKNOWN, REASON_PING_DISABLED); \
    return;                                                            \
  }

namespace safe_browsing {

void BraveFilterRequest(ClientDownloadRequest* request) {
  request->set_url("");  // URL must be present or we get a 400.
  request->clear_file_basename();
  request->clear_locale();
  request->clear_resources();  // Contains URLs and referrers
  request->clear_referrer_chain();

  // Filter binaries within archives.
  for (int i = 0; i < request->archived_binary_size(); i++) {
    ClientDownloadRequest_ArchivedBinary* archived_binary =
        request->mutable_archived_binary(i);
    archived_binary->clear_file_path();
  }
}

// Growser-370: the clientreport endpoints brave_block_safebrowsing_urls.cc
// cancels (#329). Android's download check has its own host and still runs.
bool IsBlockedDownloadPingUrl(const GURL& url) {
  return url.SchemeIs("https") &&
         (url.host() == "sb-ssl.google.com" ||
          url.host() == "safebrowsing.google.com") &&
         base::StartsWith(url.path(), "/safebrowsing/clientreport/");
}

}  // namespace safe_browsing

#include <chrome/browser/safe_browsing/download_protection/check_client_download_request_base.cc>
#undef BRAVE_ON_REQUEST_BUILT_FILTER
