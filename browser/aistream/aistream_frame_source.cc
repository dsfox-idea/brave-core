/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/aistream/aistream_frame_source.h"

#include <utility>

#include "base/memory/ref_counted_memory.h"
#include "base/strings/strcat.h"
#include "chrome/common/webui_url_constants.h"
#include "url/gurl.h"

namespace aistream {

namespace {

constexpr char kApplierPath[] = "applier.js";

constexpr char kFrameHtml[] =
    "<!doctype html><html><head><meta charset=\"utf-8\">"
    "<style>html,body{margin:0;background:transparent;color:#fff;"
    "font:14px/1.4 system-ui,sans-serif;overflow:hidden}"
    "body{padding:8px 16px}</style>"
    "<script src=\"applier.js\" defer></script></head><body></body></html>";

// Takes markup only from the New Tab Page and reports the height it needs.
// The markup cannot run script of its own: CSP allows this file only, and
// markup set through innerHTML never runs its <script> elements anyway. It
// is parsed in an inert <template> first, so the elements that could leave
// the frame or load something (a meta refresh acts on insertion) are gone
// before it goes live; a link click is swallowed for the same reason.
constexpr char kApplierJs[] = R"(
const kParent = 'chrome://newtab';
const kDropped = 'script,meta,base,link,iframe,frame,object,embed,form';
const policy = trustedTypes.createPolicy('aistream', {
  createHTML: (html) => html,
});
function reportHeight() {
  parent.postMessage(
      {type: 'height', height: document.body.getBoundingClientRect().height},
      kParent);
}
function apply(markup) {
  const template = document.createElement('template');
  template.innerHTML = policy.createHTML(markup);
  template.content.querySelectorAll(kDropped).forEach((node) => node.remove());
  document.body.replaceChildren(template.content);
  reportHeight();
}
addEventListener('message', (event) => {
  if (event.origin !== kParent || event.source !== parent) {
    return;
  }
  if (event.data && event.data.type === 'markup') {
    apply(String(event.data.markup));
  }
});
addEventListener('click', (event) => {
  if (event.target.closest('a')) {
    event.preventDefault();
  }
});
new ResizeObserver(reportHeight).observe(document.body);
parent.postMessage({type: 'ready'}, kParent);
)";

bool IsApplier(const GURL& url) {
  return content::URLDataSource::URLToRequestPath(url) == kApplierPath;
}

}  // namespace

AistreamFrameSource::AistreamFrameSource() = default;
AistreamFrameSource::~AistreamFrameSource() = default;

std::string AistreamFrameSource::GetSource() {
  return kFrameUrl;
}

void AistreamFrameSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
      std::string(IsApplier(url) ? kApplierJs : kFrameHtml)));
}

std::string AistreamFrameSource::GetMimeType(const GURL& url) {
  return IsApplier(url) ? "text/javascript" : "text/html";
}

bool AistreamFrameSource::AllowCaching() {
  return false;
}

bool AistreamFrameSource::ShouldDenyXFrameOptions() {
  // Framing is decided by frame-ancestors below; X-Frame-Options: DENY would
  // refuse the New Tab Page too.
  return false;
}

std::string AistreamFrameSource::GetContentSecurityPolicy(
    network::mojom::CSPDirectiveName directive) {
  switch (directive) {
    case network::mojom::CSPDirectiveName::DefaultSrc:
      return "default-src 'none';";
    case network::mojom::CSPDirectiveName::ScriptSrc:
      return "script-src 'self';";
    case network::mojom::CSPDirectiveName::StyleSrc:
      return "style-src 'unsafe-inline';";
    case network::mojom::CSPDirectiveName::ImgSrc:
      return "img-src data:;";
    case network::mojom::CSPDirectiveName::FontSrc:
      return "font-src data:;";
    case network::mojom::CSPDirectiveName::BaseURI:
      return "base-uri 'none';";
    case network::mojom::CSPDirectiveName::FormAction:
      return "form-action 'none';";
    case network::mojom::CSPDirectiveName::FrameAncestors:
      return base::StrCat({"frame-ancestors ", chrome::kChromeUINewTabURL, ";"});
    case network::mojom::CSPDirectiveName::RequireTrustedTypesFor:
      return "require-trusted-types-for 'script';";
    case network::mojom::CSPDirectiveName::TrustedTypes:
      return "trusted-types aistream;";
    default:
      // Nothing inherited: default-src covers connect, frame, media, object
      // and worker.
      return std::string();
  }
}

}  // namespace aistream
