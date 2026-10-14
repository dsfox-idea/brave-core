/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"

#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/thread_pool.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_component_updater::LocalDataFilesObserver;
using content::BrowserThread;
using extensions::ExtensionBrowserTest;

// Growser-352: the environment Brave's own tests get - no bundled local data.
// Every test here installs its rules with InstallMockExtension(), and the
// bundled copy (growser#87), written in the background at startup, would
// otherwise replace them whenever it lands after the test's own.
BaseLocalDataFilesBrowserTest::BaseLocalDataFilesBrowserTest() {
  brave_component_updater::LocalDataFilesService::SetSkipBundledDataForTesting(
      true);
}

BaseLocalDataFilesBrowserTest::~BaseLocalDataFilesBrowserTest() {
  brave_component_updater::LocalDataFilesService::SetSkipBundledDataForTesting(
      false);
}

void BaseLocalDataFilesBrowserTest::SetUpOnMainThread() {
  MaybeInitEmbeddedTestServer();
  ExtensionBrowserTest::SetUpOnMainThread();
}

void BaseLocalDataFilesBrowserTest::PreRunTestOnMainThread() {
  ExtensionBrowserTest::PreRunTestOnMainThread();
  ASSERT_TRUE(
      g_brave_browser_process->local_data_files_service()->IsInitialized());
}

void BaseLocalDataFilesBrowserTest::GetTestDataDir(
    base::FilePath* test_data_dir) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
}

void BaseLocalDataFilesBrowserTest::MaybeInitEmbeddedTestServer() {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  test_data_dir = test_data_dir.AppendASCII(embedded_test_server_directory());
  content::SetupCrossSiteRedirector(embedded_test_server());
  embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
  ASSERT_TRUE(embedded_test_server()->Start());
  host_resolver()->AddRule("*", "127.0.0.1");
}

void BaseLocalDataFilesBrowserTest::WaitForService() {
  scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
      g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
  ASSERT_TRUE(tr_helper->Run());
  scoped_refptr<base::ThreadTestHelper> io_helper(
      new base::ThreadTestHelper(content::GetIOThreadTaskRunner({}).get()));
  ASSERT_TRUE(io_helper->Run());
  base::RunLoop().RunUntilIdle();
}

bool BaseLocalDataFilesBrowserTest::InstallMockExtension() {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  const extensions::Extension* mock_extension =
      InstallExtension(test_data_dir.AppendASCII(test_data_directory()), 1);
  if (!mock_extension)
    return false;

  if (!service()) {
    return false;
  }

  service()->OnComponentReady(mock_extension->id(), mock_extension->path(), "");
  WaitForService();

  return true;
}
