// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#ifndef BRAVE_COMPONENTS_YA_DEV_FOLDER_RENDERER_YA_DEV_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_YA_DEV_FOLDER_RENDERER_YA_DEV_RENDER_FRAME_OBSERVER_H_

#include "content/public/renderer/render_frame_observer.h"

namespace ya_dev_folder {

// Puts window.alice.dev into the main world of every frame, on every origin.
class YaDevRenderFrameObserver final : public content::RenderFrameObserver {
 public:
  static void Create(content::RenderFrame* render_frame);

  YaDevRenderFrameObserver(const YaDevRenderFrameObserver&) = delete;
  YaDevRenderFrameObserver& operator=(const YaDevRenderFrameObserver&) = delete;

  // content::RenderFrameObserver:
  void DidClearWindowObject() override;
  void OnDestruct() override;

 private:
  explicit YaDevRenderFrameObserver(content::RenderFrame* render_frame);
  ~YaDevRenderFrameObserver() override;
};

}  // namespace ya_dev_folder

#endif  // BRAVE_COMPONENTS_YA_DEV_FOLDER_RENDERER_YA_DEV_RENDER_FRAME_OBSERVER_H_
