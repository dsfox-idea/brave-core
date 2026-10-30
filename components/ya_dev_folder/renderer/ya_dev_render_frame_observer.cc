// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include "brave/components/ya_dev_folder/renderer/ya_dev_render_frame_observer.h"

#include "brave/components/ya_dev_folder/renderer/ya_dev_js_handler.h"

namespace ya_dev_folder {

// static
void YaDevRenderFrameObserver::Create(content::RenderFrame* render_frame) {
  new YaDevRenderFrameObserver(render_frame);
}

YaDevRenderFrameObserver::YaDevRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

YaDevRenderFrameObserver::~YaDevRenderFrameObserver() = default;

void YaDevRenderFrameObserver::DidClearWindowObject() {
  YaDevJsHandler::Install(render_frame());
}

void YaDevRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace ya_dev_folder
