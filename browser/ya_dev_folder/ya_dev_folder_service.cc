// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include "brave/browser/ya_dev_folder/ya_dev_folder_service.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ya_dev_folder/ya_dev_folder_store.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "ui/shell_dialogs/select_file_policy.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace ya_dev_folder {

// static
void YaDevFolderService::Create(
    content::RenderFrameHost* render_frame_host,
    mojo::PendingReceiver<mojom::YaDevFolder> receiver) {
  // Owns itself: deleted with the document or when the pipe closes.
  new YaDevFolderService(*render_frame_host, std::move(receiver));
}

YaDevFolderService::YaDevFolderService(
    content::RenderFrameHost& render_frame_host,
    mojo::PendingReceiver<mojom::YaDevFolder> receiver)
    : DocumentService(render_frame_host, std::move(receiver)) {}

YaDevFolderService::~YaDevFolderService() {
  if (dialog_) {
    dialog_->ListenerDestroyed();
  }
}

void YaDevFolderService::LinkFolder(LinkFolderCallback callback) {
  if (origin().opaque()) {
    std::move(callback).Run(MakeReply(mojom::ErrorCode::kOpaqueOrigin));
    return;
  }
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(&render_frame_host());
  if (!web_contents || dialog_) {
    // One dialog at a time per document.
    std::move(callback).Run(MakeReply(mojom::ErrorCode::kIo));
    return;
  }
  link_callback_ = std::move(callback);
  // No SelectFilePolicy: enterprise policy does not get a say in this
  // experiment.
  dialog_ = ui::SelectFileDialog::Create(this, nullptr);
  dialog_->SelectFile(ui::SelectFileDialog::SELECT_EXISTING_FOLDER,
                      std::u16string(), base::FilePath(),
                      /*file_types=*/nullptr, 0, base::FilePath::StringType(),
                      web_contents->GetTopLevelNativeWindow());
  LOG(INFO) << "ya-dev: folder dialog opened for " << origin();
}

void YaDevFolderService::FileSelected(const ui::SelectedFileInfo& file,
                                      int index) {
  FinishLink(mojom::ErrorCode::kOk, file.path());
}

void YaDevFolderService::FileSelectionCanceled() {
  FinishLink(mojom::ErrorCode::kCancelled, base::FilePath());
}

void YaDevFolderService::FinishLink(mojom::ErrorCode error,
                                    const base::FilePath& folder) {
  dialog_.reset();
  mojom::ReplyPtr reply = MakeReply(error);
  if (error == mojom::ErrorCode::kOk) {
    YaDevFolderStore::Get(render_frame_host().GetBrowserContext())
        .Link(origin(), folder);
    reply->name = folder.BaseName().AsUTF8Unsafe();
    LOG(INFO) << "ya-dev: " << origin() << " linked " << folder;
  }
  if (link_callback_) {
    std::move(link_callback_).Run(std::move(reply));
  }
}

void YaDevFolderService::UnlinkFolder(UnlinkFolderCallback callback) {
  YaDevFolderStore::Get(render_frame_host().GetBrowserContext())
      .Unlink(origin());
  std::move(callback).Run(MakeReply(mojom::ErrorCode::kOk));
}

void YaDevFolderService::Dispatch(Job job, ReplyCallback callback) {
  std::optional<base::FilePath> root =
      YaDevFolderStore::Get(render_frame_host().GetBrowserContext())
          .Find(origin());
  if (!root) {
    std::move(callback).Run(MakeReply(mojom::ErrorCode::kNotLinked));
    return;
  }
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(
          [](base::FilePath folder, Job run) {
            return std::move(run).Run(FolderSandbox(std::move(folder)));
          },
          *root, std::move(job)),
      std::move(callback));
}

void YaDevFolderService::List(const std::string& path, ListCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string path, const FolderSandbox& sandbox) {
                 return sandbox.List(path);
               },
               path),
           std::move(callback));
}

void YaDevFolderService::Stat(const std::string& path, StatCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string path, const FolderSandbox& sandbox) {
                 return sandbox.Stat(path);
               },
               path),
           std::move(callback));
}

void YaDevFolderService::Read(const std::string& path, ReadCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string path, const FolderSandbox& sandbox) {
                 return sandbox.Read(path);
               },
               path),
           std::move(callback));
}

void YaDevFolderService::Write(const std::string& path,
                               mojo_base::BigBuffer data,
                               WriteCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string path, mojo_base::BigBuffer data,
                  const FolderSandbox& sandbox) {
                 return sandbox.Write(path, data.byte_span());
               },
               path, std::move(data)),
           std::move(callback));
}

void YaDevFolderService::MakeDirectory(const std::string& path,
                                       MakeDirectoryCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string path, const FolderSandbox& sandbox) {
                 return sandbox.MakeDirectory(path);
               },
               path),
           std::move(callback));
}

void YaDevFolderService::Remove(const std::string& path,
                                bool recursive,
                                RemoveCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string path, bool recursive,
                  const FolderSandbox& sandbox) {
                 return sandbox.Remove(path, recursive);
               },
               path, recursive),
           std::move(callback));
}

void YaDevFolderService::Move(const std::string& from,
                              const std::string& to,
                              MoveCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string from, std::string to,
                  const FolderSandbox& sandbox) {
                 return sandbox.Move(from, to);
               },
               from, to),
           std::move(callback));
}

void YaDevFolderService::Copy(const std::string& from,
                              const std::string& to,
                              CopyCallback callback) {
  Dispatch(base::BindOnce(
               [](std::string from, std::string to,
                  const FolderSandbox& sandbox) {
                 return sandbox.Copy(from, to);
               },
               from, to),
           std::move(callback));
}

}  // namespace ya_dev_folder
