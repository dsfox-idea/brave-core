/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/tile_icon_fetcher.h"

#include <optional>
#include <utility>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/storage_partition.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"

namespace brave_new_tab {

namespace {

// When each host was last asked, so a site that has no such icon is not asked
// again on every new tab.
constexpr char kAttemptsPref[] = "growser.tile_icons.attempts";

// The convention Safari established and most sites follow whether or not they
// declare it in their markup.
constexpr char kTouchIconPath[] = "/apple-touch-icon.png";

// An icon larger than this is not an icon.
constexpr size_t kMaxIconBytes = 1024 * 1024;

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("growser_tile_icon", R"(
      semantics {
        sender: "Growser new tab board"
        description:
          "Fetches the large icon a site publishes at the conventional "
          "/apple-touch-icon.png path, so that the new tab board can draw it "
          "sharply. Only for sites already on the board, and only when the "
          "browser holds no large icon for them."
        trigger:
          "Showing the new tab board for a site whose icon is too small to "
          "draw, at most once every three weeks per site."
        data: "None. The request carries no cookies and no referrer."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "Turning off the top sites board on the new tab page stops this."
        policy_exception_justification: "Not implemented."
      })");

}  // namespace

GURL TouchIconURLFor(const GURL& page_url) {
  if (!page_url.SchemeIsHTTPOrHTTPS()) {
    return GURL();
  }
  GURL::Replacements replacements;
  replacements.SetPathStr(kTouchIconPath);
  replacements.ClearQuery();
  replacements.ClearRef();
  return page_url.ReplaceComponents(replacements);
}

bool AttemptHasLapsed(base::Time last, base::Time now) {
  // A clock that has moved backwards would otherwise pin a site to "asked
  // recently" until it caught up again.
  return now < last || now - last >= TileIconFetcher::kRetryInterval;
}

TileIconFetcher::TileIconFetcher(Profile* profile) : profile_(profile) {}

TileIconFetcher::~TileIconFetcher() = default;

// static
TileIconFetcher* TileIconFetcher::GetForProfile(Profile* profile) {
  static constexpr char kUserDataKey[] = "growser_tile_icon_fetcher";
  if (!profile) {
    return nullptr;
  }
  auto* existing =
      static_cast<TileIconFetcher*>(profile->GetUserData(kUserDataKey));
  if (existing) {
    return existing;
  }
  auto fetcher = base::WrapUnique(new TileIconFetcher(profile));
  auto* raw = fetcher.get();
  profile->SetUserData(kUserDataKey, std::move(fetcher));
  return raw;
}

// static
void TileIconFetcher::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(kAttemptsPref);
}

void TileIconFetcher::MaybeFetch(const GURL& page_url) {
  if (!TouchIconURLFor(page_url).is_valid() ||
      !ShouldTry(std::string(page_url.host()))) {
    return;
  }
  // Recorded on entry rather than when its turn comes, so that a site queued
  // twice in one sitting - the board asks again whenever it repaints - is not
  // asked twice.
  RecordAttempt(std::string(page_url.host()));
  pending_.push_back(page_url);
  StartNext();
}

void TileIconFetcher::StartNext() {
  if (loader_ || pending_.empty()) {
    return;
  }
  const GURL page_url = pending_.front();
  pending_.erase(pending_.begin());
  const GURL icon_url = TouchIconURLFor(page_url);

  auto request = std::make_unique<network::ResourceRequest>();
  request->url = icon_url;
  request->method = "GET";
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->referrer_policy = net::ReferrerPolicy::NO_REFERRER;

  loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);
  loader_->DownloadToString(
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess()
          .get(),
      base::BindOnce(&TileIconFetcher::OnResponse, weak_factory_.GetWeakPtr(),
                     page_url, icon_url),
      kMaxIconBytes);
}

bool TileIconFetcher::ShouldTry(const std::string& host) const {
  if (host.empty()) {
    return false;
  }
  const base::DictValue& attempts =
      profile_->GetPrefs()->GetDict(kAttemptsPref);
  const std::string* stamp = attempts.FindString(host);
  if (!stamp) {
    return true;
  }
  int64_t microseconds = 0;
  if (!base::StringToInt64(*stamp, &microseconds)) {
    return true;
  }
  return AttemptHasLapsed(
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(microseconds)),
      base::Time::Now());
}

void TileIconFetcher::RecordAttempt(const std::string& host) {
  // Recorded before the request rather than after it, so a site that times out
  // or errors is not asked again on the next new tab.
  ScopedDictPrefUpdate update(profile_->GetPrefs(), kAttemptsPref);
  update->Set(host, base::NumberToString(
                        base::Time::Now().ToDeltaSinceWindowsEpoch()
                            .InMicroseconds()));
}

void TileIconFetcher::OnResponse(GURL page_url,
                                 GURL icon_url,
                                 std::optional<std::string> body) {
  loader_.reset();
  StartNext();
  if (!body || body->empty()) {
    return;
  }
  // Held by pointer so the bytes survive the decode and can be stored exactly
  // as they arrived, rather than re-encoded from the decoded bitmap.
  auto bytes = std::make_unique<std::string>(*std::move(body));
  // Taken before the call rather than inside it: the argument order of a call
  // is unspecified in C++, and `std::move(bytes)` in the same argument list
  // could otherwise be evaluated first. The span survives the move because
  // moving a unique_ptr moves the pointer, not the string it points at.
  const base::span<const uint8_t> data = base::as_byte_span(*bytes);
  // Decoded out of process: this is a bitmap from a site, and the browser
  // process is the last place to parse one.
  data_decoder::DecodeImageIsolated(
      data, data_decoder::mojom::ImageCodec::kDefault,
      /*shrink_to_fit=*/false, kMaxIconBytes, gfx::Size(),
      base::BindOnce(&TileIconFetcher::OnDecoded, weak_factory_.GetWeakPtr(),
                     std::move(page_url), std::move(icon_url),
                     std::move(bytes)));
}

void TileIconFetcher::OnDecoded(GURL page_url,
                                GURL icon_url,
                                std::unique_ptr<std::string> body,
                                const SkBitmap& bitmap) {
  if (bitmap.isNull() || bitmap.width() < kWantedIconSize) {
    // Nothing there, or nothing better than what the board already has.
    return;
  }
  favicon::FaviconService* favicon_service =
      FaviconServiceFactory::GetForProfile(profile_,
                                           ServiceAccessType::EXPLICIT_ACCESS);
  if (!favicon_service) {
    return;
  }
  // Merge rather than set: the page already has its small favicon and the tab
  // strip still wants it. SetOnDemandFavicons would refuse outright for a page
  // that holds any unexpired icon, which every board site does.
  favicon_service->MergeFavicon(
      page_url, icon_url, favicon_base::IconType::kTouchIcon,
      base::MakeRefCounted<base::RefCountedString>(std::move(*body)),
      gfx::Size(bitmap.width(), bitmap.height()));
}

}  // namespace brave_new_tab
