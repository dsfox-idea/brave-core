// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#ifndef BRAVE_COMPONENTS_YA_DEV_FOLDER_RENDERER_YA_DEV_JS_HANDLER_H_
#define BRAVE_COMPONENTS_YA_DEV_FOLDER_RENDERER_YA_DEV_JS_HANDLER_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/ya_dev_folder/common/ya_dev_folder.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace ya_dev_folder {

// window.alice.dev: a folder on the user's disk, for any page on any origin.
// Every method returns a Promise. A failure rejects with an Error whose `code`
// is one of the strings in ErrorName() (ya_dev_js_handler.cc).
//
//   linkFolder()                -> {name}      asks the user to pick a folder
//   unlinkFolder()              -> undefined
//   listFiles(path = '')        -> [{name, kind, size, modified}]
//   stat(path)                  -> {name, kind, size, modified}
//   readFile(path)              -> Uint8Array
//   readText(path)              -> string (UTF-8)
//   writeFile(path, data)       -> undefined   data: string | ArrayBuffer | view
//   mkdir(path)                 -> undefined   creates parents too
//   remove(path, {recursive})   -> undefined
//   move(from, to)              -> undefined
//   copy(from, to)              -> undefined
class YaDevJsHandler final : public gin::Wrappable<YaDevJsHandler>,
                             public content::RenderFrameObserver {
 public:
  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::kYaDevBindings};

  explicit YaDevJsHandler(content::RenderFrame* render_frame);
  YaDevJsHandler(const YaDevJsHandler&) = delete;
  YaDevJsHandler& operator=(const YaDevJsHandler&) = delete;
  ~YaDevJsHandler() override;

  static void Install(content::RenderFrame* render_frame);

  // content::RenderFrameObserver:
  void OnDestruct() override;

 private:
  // What the resolved value of a call looks like.
  enum class Shape { kNothing, kName, kEntries, kEntry, kBytes, kText };
  using Call = base::OnceCallback<void(base::OnceCallback<void(mojom::ReplyPtr)>)>;

  // gin::WrappableBase:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;

  v8::Local<v8::Promise> LinkFolder(gin::Arguments* args);
  v8::Local<v8::Promise> UnlinkFolder(gin::Arguments* args);
  v8::Local<v8::Promise> ListFiles(gin::Arguments* args);
  v8::Local<v8::Promise> Stat(gin::Arguments* args);
  v8::Local<v8::Promise> ReadFile(gin::Arguments* args);
  v8::Local<v8::Promise> ReadText(gin::Arguments* args);
  v8::Local<v8::Promise> WriteFile(gin::Arguments* args);
  v8::Local<v8::Promise> MakeDirectory(gin::Arguments* args);
  v8::Local<v8::Promise> Remove(gin::Arguments* args);
  v8::Local<v8::Promise> Move(gin::Arguments* args);
  v8::Local<v8::Promise> Copy(gin::Arguments* args);

  bool EnsureConnected();
  v8::Local<v8::Promise> Run(v8::Isolate* isolate, Shape shape, Call call);
  void OnReply(v8::Isolate* isolate,
               v8::Global<v8::Promise::Resolver> resolver,
               v8::Global<v8::Context> context,
               Shape shape,
               mojom::ReplyPtr reply);

  mojo::Remote<mojom::YaDevFolder> remote_;
};

}  // namespace ya_dev_folder

#endif  // BRAVE_COMPONENTS_YA_DEV_FOLDER_RENDERER_YA_DEV_JS_HANDLER_H_
