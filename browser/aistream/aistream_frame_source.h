/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_AISTREAM_AISTREAM_FRAME_SOURCE_H_
#define BRAVE_BROWSER_AISTREAM_AISTREAM_FRAME_SOURCE_H_

#include <string>

#include "content/public/browser/url_data_source.h"

namespace aistream {

inline constexpr char kFrameUrl[] = "chrome-untrusted://aistream-widget/";

// growser#422: the frame a widget's markup lives in. The markup is the
// agent's, so the frame runs no script but our applier, loads nothing from
// the network, and can only be embedded by the New Tab Page.
class AistreamFrameSource final : public content::URLDataSource {
 public:
  AistreamFrameSource();
  AistreamFrameSource(const AistreamFrameSource&) = delete;
  AistreamFrameSource& operator=(const AistreamFrameSource&) = delete;
  ~AistreamFrameSource() override;

  // content::URLDataSource:
  std::string GetSource() override;
  void StartDataRequest(const GURL& url,
                        const content::WebContents::Getter& wc_getter,
                        GotDataCallback callback) override;
  std::string GetMimeType(const GURL& url) override;
  bool AllowCaching() override;
  bool ShouldDenyXFrameOptions() override;
  std::string GetContentSecurityPolicy(
      network::mojom::CSPDirectiveName directive) override;
};

}  // namespace aistream

#endif  // BRAVE_BROWSER_AISTREAM_AISTREAM_FRAME_SOURCE_H_
