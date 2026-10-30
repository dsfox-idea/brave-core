// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include "brave/components/ya_dev_folder/renderer/ya_dev_js_handler.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/strings/string_view_util.h"
#include "base/functional/bind.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "gin/object_template_builder.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace ya_dev_folder {

namespace {

const char* ErrorName(mojom::ErrorCode code) {
  switch (code) {
    case mojom::ErrorCode::kOk:
      return "ok";
    case mojom::ErrorCode::kNotLinked:
      return "not-linked";
    case mojom::ErrorCode::kOpaqueOrigin:
      return "opaque-origin";
    case mojom::ErrorCode::kCancelled:
      return "cancelled";
    case mojom::ErrorCode::kInvalidPath:
      return "invalid-path";
    case mojom::ErrorCode::kSymlink:
      return "symlink";
    case mojom::ErrorCode::kNotFound:
      return "not-found";
    case mojom::ErrorCode::kAlreadyExists:
      return "already-exists";
    case mojom::ErrorCode::kNotADirectory:
      return "not-a-directory";
    case mojom::ErrorCode::kIsADirectory:
      return "is-a-directory";
    case mojom::ErrorCode::kTooLarge:
      return "too-large";
    case mojom::ErrorCode::kIo:
      return "io-error";
  }
  return "io-error";
}

v8::Local<v8::Value> MakeError(v8::Isolate* isolate,
                               v8::Local<v8::Context> context,
                               const std::string& code) {
  v8::Local<v8::Value> error =
      v8::Exception::Error(gin::StringToV8(isolate, "alice.dev: " + code));
  std::ignore = error.As<v8::Object>()->Set(
      context, gin::StringToSymbol(isolate, "code"),
      gin::StringToV8(isolate, code));
  return error;
}

v8::Local<v8::Promise> Rejected(v8::Isolate* isolate, const std::string& code) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }
  std::ignore = resolver->Reject(context, MakeError(isolate, context, code));
  return resolver->GetPromise();
}

v8::Local<v8::Value> EntryToV8(v8::Isolate* isolate, const mojom::Entry& entry) {
  return gin::DataObjectBuilder(isolate)
      .Set("name", entry.name)
      .Set("kind", std::string(entry.is_directory ? "directory" : "file"))
      .Set("size", static_cast<double>(entry.size))
      .Set("modified", entry.modified_ms)
      .Build();
}

v8::Local<v8::Value> BytesToV8(v8::Isolate* isolate,
                               const mojo_base::BigBuffer& data) {
  v8::Local<v8::ArrayBuffer> buffer =
      v8::ArrayBuffer::New(isolate, data.size());
  base::span(buffer->GetBackingStore()->ByteSpan())
      .copy_from(data.byte_span());
  return v8::Uint8Array::New(buffer, 0, data.size());
}

// A required string argument, or an optional one that may be absent or
// undefined. Anything else (a number, null, an object) is refused, not coerced.
bool NextString(gin::Arguments* args, bool optional, std::string* out) {
  v8::Local<v8::Value> value = args->PeekNext();
  if (value.IsEmpty() || value->IsUndefined()) {
    out->clear();
    return optional;
  }
  return value->IsString() && args->GetNext(out);
}

bool ToBytes(v8::Isolate* isolate,
             v8::Local<v8::Value> value,
             std::vector<uint8_t>* out) {
  if (value->IsString()) {
    std::string utf8;
    if (!gin::ConvertFromV8(isolate, value, &utf8)) {
      return false;
    }
    out->assign(utf8.begin(), utf8.end());
    return true;
  }
  if (value->IsArrayBuffer()) {
    auto bytes = value.As<v8::ArrayBuffer>()->GetBackingStore()->ByteSpan();
    out->assign(bytes.begin(), bytes.end());
    return true;
  }
  if (value->IsArrayBufferView()) {
    auto view = value.As<v8::ArrayBufferView>();
    out->resize(view->ByteLength());
    return view->CopyContents(out->data(), out->size()) == out->size();
  }
  return false;
}

}  // namespace

YaDevJsHandler::YaDevJsHandler(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

YaDevJsHandler::~YaDevJsHandler() = default;

// static
void YaDevJsHandler::Install(content::RenderFrame* render_frame) {
  CHECK(render_frame);
  v8::Isolate* isolate =
      render_frame->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  // window.alice
  v8::Local<v8::Value> alice_value;
  v8::Local<v8::Object> alice;
  if (!global->Get(context, gin::StringToV8(isolate, "alice"))
           .ToLocal(&alice_value) ||
      !alice_value->IsObject()) {
    alice = v8::Object::New(isolate);
    std::ignore =
        global->Set(context, gin::StringToSymbol(isolate, "alice"), alice);
  } else {
    alice = alice_value.As<v8::Object>();
  }

  // window.alice.dev: not writable, not configurable, so a page cannot swap it.
  YaDevJsHandler* handler = cppgc::MakeGarbageCollected<YaDevJsHandler>(
      isolate->GetCppHeap()->GetAllocationHandle(), render_frame);
  v8::PropertyDescriptor dev(handler->GetWrapper(isolate).ToLocalChecked(),
                             false);
  dev.set_configurable(false);
  std::ignore =
      alice->DefineProperty(context, gin::StringToV8(isolate, "dev"), dev);
}

void YaDevJsHandler::OnDestruct() {}

gin::ObjectTemplateBuilder YaDevJsHandler::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<YaDevJsHandler>::GetObjectTemplateBuilder(isolate)
      .SetMethod("linkFolder", &YaDevJsHandler::LinkFolder)
      .SetMethod("unlinkFolder", &YaDevJsHandler::UnlinkFolder)
      .SetMethod("listFiles", &YaDevJsHandler::ListFiles)
      .SetMethod("stat", &YaDevJsHandler::Stat)
      .SetMethod("readFile", &YaDevJsHandler::ReadFile)
      .SetMethod("readText", &YaDevJsHandler::ReadText)
      .SetMethod("writeFile", &YaDevJsHandler::WriteFile)
      .SetMethod("mkdir", &YaDevJsHandler::MakeDirectory)
      .SetMethod("remove", &YaDevJsHandler::Remove)
      .SetMethod("move", &YaDevJsHandler::Move)
      .SetMethod("copy", &YaDevJsHandler::Copy);
}

const gin::WrapperInfo* YaDevJsHandler::wrapper_info() const {
  return &kWrapperInfo;
}

bool YaDevJsHandler::EnsureConnected() {
  if (!remote_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker().GetInterface(
        remote_.BindNewPipeAndPassReceiver());
    remote_.reset_on_disconnect();
  }
  return remote_.is_bound();
}

v8::Local<v8::Promise> YaDevJsHandler::Run(v8::Isolate* isolate,
                                           Shape shape,
                                           Call call) {
  if (!EnsureConnected()) {
    return Rejected(isolate, "io-error");
  }
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    return v8::Local<v8::Promise>();
  }
  std::move(call).Run(base::BindOnce(
      &YaDevJsHandler::OnReply, base::Unretained(this), isolate,
      v8::Global<v8::Promise::Resolver>(isolate, resolver),
      v8::Global<v8::Context>(isolate, context), shape));
  return resolver->GetPromise();
}

void YaDevJsHandler::OnReply(v8::Isolate* isolate,
                             v8::Global<v8::Promise::Resolver> resolver,
                             v8::Global<v8::Context> context_global,
                             Shape shape,
                             mojom::ReplyPtr reply) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = context_global.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Promise::Resolver> local = resolver.Get(isolate);

  if (reply->error != mojom::ErrorCode::kOk) {
    std::ignore = local->Reject(
        context, MakeError(isolate, context, ErrorName(reply->error)));
    return;
  }

  v8::Local<v8::Value> value = v8::Undefined(isolate);
  switch (shape) {
    case Shape::kNothing:
      break;
    case Shape::kName:
      value = gin::DataObjectBuilder(isolate)
                  .Set("name", reply->name.value_or(std::string()))
                  .Build();
      break;
    case Shape::kEntries: {
      const size_t count = reply->entries ? reply->entries->size() : 0;
      v8::Local<v8::Array> list = v8::Array::New(isolate, count);
      for (size_t index = 0; index < count; ++index) {
        std::ignore = list->Set(context, index,
                                EntryToV8(isolate, *(*reply->entries)[index]));
      }
      value = list;
      break;
    }
    case Shape::kEntry:
      if (reply->entry) {
        value = EntryToV8(isolate, *reply->entry);
      }
      break;
    case Shape::kBytes:
      if (reply->data) {
        value = BytesToV8(isolate, *reply->data);
      }
      break;
    case Shape::kText:
      if (reply->data) {
        value = gin::StringToV8(isolate,
                                base::as_string_view(reply->data->byte_span()));
      }
      break;
  }
  std::ignore = local->Resolve(context, value);
}

v8::Local<v8::Promise> YaDevJsHandler::LinkFolder(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return Rejected(args->isolate(), "io-error");
  }
  return Run(args->isolate(), Shape::kName,
             base::BindOnce(&mojom::YaDevFolder::LinkFolder,
                            base::Unretained(remote_.get())));
}

v8::Local<v8::Promise> YaDevJsHandler::UnlinkFolder(gin::Arguments* args) {
  if (!EnsureConnected()) {
    return Rejected(args->isolate(), "io-error");
  }
  return Run(args->isolate(), Shape::kNothing,
             base::BindOnce(&mojom::YaDevFolder::UnlinkFolder,
                            base::Unretained(remote_.get())));
}

v8::Local<v8::Promise> YaDevJsHandler::ListFiles(gin::Arguments* args) {
  std::string path;
  if (!NextString(args, /*optional=*/true, &path) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kEntries,
             base::BindOnce(&mojom::YaDevFolder::List,
                            base::Unretained(remote_.get()), path));
}

v8::Local<v8::Promise> YaDevJsHandler::Stat(gin::Arguments* args) {
  std::string path;
  if (!NextString(args, /*optional=*/false, &path) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kEntry,
             base::BindOnce(&mojom::YaDevFolder::Stat,
                            base::Unretained(remote_.get()), path));
}

v8::Local<v8::Promise> YaDevJsHandler::ReadFile(gin::Arguments* args) {
  std::string path;
  if (!NextString(args, /*optional=*/false, &path) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kBytes,
             base::BindOnce(&mojom::YaDevFolder::Read,
                            base::Unretained(remote_.get()), path));
}

v8::Local<v8::Promise> YaDevJsHandler::ReadText(gin::Arguments* args) {
  std::string path;
  if (!NextString(args, /*optional=*/false, &path) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kText,
             base::BindOnce(&mojom::YaDevFolder::Read,
                            base::Unretained(remote_.get()), path));
}

v8::Local<v8::Promise> YaDevJsHandler::WriteFile(gin::Arguments* args) {
  std::string path;
  std::vector<uint8_t> bytes;
  v8::Local<v8::Value> data;
  if (!NextString(args, /*optional=*/false, &path) || !args->GetNext(&data) ||
      !ToBytes(args->isolate(), data, &bytes) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kNothing,
             base::BindOnce(&mojom::YaDevFolder::Write,
                            base::Unretained(remote_.get()), path,
                            mojo_base::BigBuffer(bytes)));
}

v8::Local<v8::Promise> YaDevJsHandler::MakeDirectory(gin::Arguments* args) {
  std::string path;
  if (!NextString(args, /*optional=*/false, &path) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kNothing,
             base::BindOnce(&mojom::YaDevFolder::MakeDirectory,
                            base::Unretained(remote_.get()), path));
}

v8::Local<v8::Promise> YaDevJsHandler::Remove(gin::Arguments* args) {
  std::string path;
  if (!NextString(args, /*optional=*/false, &path)) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  bool recursive = false;
  v8::Local<v8::Value> options = args->PeekNext();
  if (!options.IsEmpty() && !options->IsUndefined()) {
    gin::Dictionary dictionary(args->isolate());
    if (!gin::ConvertFromV8(args->isolate(), options, &dictionary)) {
      return Rejected(args->isolate(), "invalid-argument");
    }
    dictionary.Get("recursive", &recursive);
  }
  if (!EnsureConnected()) {
    return Rejected(args->isolate(), "io-error");
  }
  return Run(args->isolate(), Shape::kNothing,
             base::BindOnce(&mojom::YaDevFolder::Remove,
                            base::Unretained(remote_.get()), path, recursive));
}

v8::Local<v8::Promise> YaDevJsHandler::Move(gin::Arguments* args) {
  std::string from;
  std::string to;
  if (!NextString(args, /*optional=*/false, &from) ||
      !NextString(args, /*optional=*/false, &to) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kNothing,
             base::BindOnce(&mojom::YaDevFolder::Move,
                            base::Unretained(remote_.get()), from, to));
}

v8::Local<v8::Promise> YaDevJsHandler::Copy(gin::Arguments* args) {
  std::string from;
  std::string to;
  if (!NextString(args, /*optional=*/false, &from) ||
      !NextString(args, /*optional=*/false, &to) || !EnsureConnected()) {
    return Rejected(args->isolate(), "invalid-argument");
  }
  return Run(args->isolate(), Shape::kNothing,
             base::BindOnce(&mojom::YaDevFolder::Copy,
                            base::Unretained(remote_.get()), from, to));
}

}  // namespace ya_dev_folder
