// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#ifndef BRAVE_BROWSER_YA_DEV_FOLDER_YA_DEV_FOLDER_SERVICE_H_
#define BRAVE_BROWSER_YA_DEV_FOLDER_YA_DEV_FOLDER_SERVICE_H_

#include <string>

#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ya_dev_folder/browser/folder_sandbox.h"
#include "brave/components/ya_dev_folder/common/ya_dev_folder.mojom.h"
#include "content/public/browser/document_service.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace ya_dev_folder {

// The browser end of window.alice.dev, one per document. It is bound for every
// origin: that is the experiment. The folder belongs to the document's origin.
class YaDevFolderService final
    : public content::DocumentService<mojom::YaDevFolder>,
      public ui::SelectFileDialog::Listener {
 public:
  static void Create(content::RenderFrameHost* render_frame_host,
                     mojo::PendingReceiver<mojom::YaDevFolder> receiver);

  YaDevFolderService(const YaDevFolderService&) = delete;
  YaDevFolderService& operator=(const YaDevFolderService&) = delete;

  // mojom::YaDevFolder:
  void LinkFolder(LinkFolderCallback callback) override;
  void UnlinkFolder(UnlinkFolderCallback callback) override;
  void List(const std::string& path, ListCallback callback) override;
  void Stat(const std::string& path, StatCallback callback) override;
  void Read(const std::string& path, ReadCallback callback) override;
  void Write(const std::string& path,
             mojo_base::BigBuffer data,
             WriteCallback callback) override;
  void MakeDirectory(const std::string& path,
                     MakeDirectoryCallback callback) override;
  void Remove(const std::string& path,
              bool recursive,
              RemoveCallback callback) override;
  void Move(const std::string& from,
            const std::string& to,
            MoveCallback callback) override;
  void Copy(const std::string& from,
            const std::string& to,
            CopyCallback callback) override;

  // ui::SelectFileDialog::Listener:
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void FileSelectionCanceled() override;

 private:
  using ReplyCallback = base::OnceCallback<void(mojom::ReplyPtr)>;
  using Job = base::OnceCallback<mojom::ReplyPtr(const FolderSandbox&)>;

  YaDevFolderService(content::RenderFrameHost& render_frame_host,
                     mojo::PendingReceiver<mojom::YaDevFolder> receiver);
  ~YaDevFolderService() override;

  // Runs `job` on the linked folder off the UI thread, or answers kNotLinked.
  void Dispatch(Job job, ReplyCallback callback);
  void FinishLink(mojom::ErrorCode error, const base::FilePath& folder);

  scoped_refptr<ui::SelectFileDialog> dialog_;
  ReplyCallback link_callback_;
  base::WeakPtrFactory<YaDevFolderService> weak_factory_{this};
};

}  // namespace ya_dev_folder

#endif  // BRAVE_BROWSER_YA_DEV_FOLDER_YA_DEV_FOLDER_SERVICE_H_
