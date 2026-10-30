// Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
// Growser-ya-dev (growser#397): experiment code, never ships.

#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/shell_dialogs/fake_select_file_dialog.h"

namespace ya_dev_folder {

// window.alice.dev from pages on unrelated origins, against a real folder.
class YaDevFolderBrowserTest : public PlatformBrowserTest {
 public:
  void SetUpOnMainThread() override {
    PlatformBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(temp_.CreateUniqueTempDir());
    folder_ = temp_.GetPath().AppendASCII("shared");
    ASSERT_TRUE(base::CreateDirectory(folder_));
    factory_ = ui::FakeSelectFileDialog::RegisterFactory();
  }

  content::WebContents* web_contents() {
    return chrome_test_utils::GetActiveWebContents(this);
  }

  void Open(const std::string& host) {
    ASSERT_TRUE(content::NavigateToURL(
        web_contents(), embedded_test_server()->GetURL(host, "/empty.html")));
  }

  // Runs `script` (an async function body) in the page; returns its result.
  content::EvalJsResult Run(const std::string& script) {
    return content::EvalJs(web_contents(),
                           "(async () => {" + script + "})()");
  }

  // The user picks `folder_` when the dialog opens.
  void PickFolderOnOpen() {
    factory_->SetOpenCallback(base::BindLambdaForTesting([this] {
      EXPECT_TRUE(factory_->GetLastDialog()->CallFileSelected(folder_, ""));
    }));
  }

  base::ScopedTempDir temp_;
  base::FilePath folder_;
  raw_ptr<ui::FakeSelectFileDialog::Factory, DanglingUntriaged> factory_ =
      nullptr;
};

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, ApiIsOnEveryOrigin) {
  for (const char* host : {"a.test", "b.example", "localhost"}) {
    Open(host);
    EXPECT_EQ("function",
              content::EvalJs(web_contents(),
                              "typeof window.alice.dev.linkFolder"))
        << host;
  }
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, PageCannotReplaceTheApi) {
  Open("a.test");
  EXPECT_EQ("function",
            content::EvalJs(web_contents(),
                            "window.alice.dev = 1; typeof window.alice.dev.stat"));
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, NotLinkedUntilTheUserPicks) {
  Open("a.test");
  EXPECT_EQ("not-linked",
            Run("try { await alice.dev.listFiles(); } catch (e) { return e.code; }"));
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, CancelledDialogRejects) {
  Open("a.test");
  factory_->SetOpenCallback(base::BindLambdaForTesting(
      [this] { factory_->GetLastDialog()->CallFileSelectionCanceled(); }));
  EXPECT_EQ("cancelled",
            Run("try { await alice.dev.linkFolder(); } catch (e) { return e.code; }"));
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, FullRoundTrip) {
  Open("a.test");
  PickFolderOnOpen();
  EXPECT_EQ("shared", Run("return (await alice.dev.linkFolder()).name;"));

  EXPECT_EQ("hello", Run(R"(
    await alice.dev.writeFile('notes/a.txt', 'hello');
    return await alice.dev.readText('notes/a.txt');
  )"));
  std::string on_disk;
  ASSERT_TRUE(base::ReadFileToString(
      folder_.AppendASCII("notes").AppendASCII("a.txt"), &on_disk));
  EXPECT_EQ("hello", on_disk);

  EXPECT_EQ("1,2,255", Run(R"(
    await alice.dev.writeFile('b.bin', new Uint8Array([1, 2, 255]));
    return Array.from(await alice.dev.readFile('b.bin')).join();
  )"));

  EXPECT_EQ("a.txt:file:5", Run(R"(
    const list = await alice.dev.listFiles('notes');
    return list.map(e => e.name + ':' + e.kind + ':' + e.size).join();
  )"));
  EXPECT_EQ("directory", Run("return (await alice.dev.stat('notes')).kind;"));

  EXPECT_EQ("hello", Run(R"(
    await alice.dev.mkdir('deep/er');
    await alice.dev.copy('notes/a.txt', 'deep/er/c.txt');
    await alice.dev.move('deep/er/c.txt', 'deep/m.txt');
    return await alice.dev.readText('deep/m.txt');
  )"));

  EXPECT_EQ("not-found", Run(R"(
    await alice.dev.remove('deep', {recursive: true});
    try { await alice.dev.stat('deep'); } catch (e) { return e.code; }
  )"));
  EXPECT_FALSE(base::PathExists(folder_.AppendASCII("deep")));
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, PathsAndArgumentsAreChecked) {
  Open("a.test");
  PickFolderOnOpen();
  EXPECT_EQ("shared", Run("return (await alice.dev.linkFolder()).name;"));
  EXPECT_EQ("invalid-path,invalid-path,invalid-path", Run(R"(
    const codes = [];
    for (const p of ['../x', '/etc/passwd', 'C:/Windows']) {
      try { await alice.dev.readFile(p); } catch (e) { codes.push(e.code); }
    }
    return codes.join();
  )"));
  EXPECT_EQ("invalid-argument,invalid-argument,invalid-argument", Run(R"(
    const codes = [];
    for (const call of [() => alice.dev.readFile(), () => alice.dev.readFile(5),
                        () => alice.dev.writeFile('x', {})]) {
      try { await call(); } catch (e) { codes.push(e.code); }
    }
    return codes.join();
  )"));
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, LinkBelongsToTheOrigin) {
  Open("a.test");
  PickFolderOnOpen();
  EXPECT_EQ("shared", Run("return (await alice.dev.linkFolder()).name;"));
  Open("b.example");
  EXPECT_EQ("not-linked",
            Run("try { await alice.dev.listFiles(); } catch (e) { return e.code; }"));
  Open("a.test");
  EXPECT_EQ("0", Run("return String((await alice.dev.listFiles()).length);"));
  EXPECT_EQ("not-linked", Run(R"(
    await alice.dev.unlinkFolder();
    try { await alice.dev.listFiles(); } catch (e) { return e.code; }
  )"));
}

IN_PROC_BROWSER_TEST_F(YaDevFolderBrowserTest, OpaqueOriginCannotLink) {
  ASSERT_TRUE(content::NavigateToURL(web_contents(), GURL("data:text/html,x")));
  EXPECT_EQ("opaque-origin",
            Run("try { await alice.dev.linkFolder(); } catch (e) { return e.code; }"));
}

}  // namespace ya_dev_folder
