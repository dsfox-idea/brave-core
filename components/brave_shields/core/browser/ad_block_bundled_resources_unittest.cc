// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// growser (#87): the scriptlet resources are bundled with the build because
// their component is served by go-updater.brave.com, which answers a fork
// "403 Missing auth header". The build being green says nothing about whether
// the vendored file is still the shape the engine can read - a rename or a
// format change upstream would leave every ##+js() rule silently inert, which
// is exactly the failure this issue was about in the first place.
//
// So this reads the vendored file and asserts the engine makes resources out
// of it. Refreshed by scripts/update-vendored-data.py.

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

namespace {

base::FilePath VendoredResourcesPath() {
  base::FilePath root;
  base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &root);
  return root.AppendASCII("brave")
      .AppendASCII("components")
      .AppendASCII("brave_shields")
      .AppendASCII("resources")
      .AppendASCII("adblock")
      .AppendASCII("resources.json");
}

}  // namespace

TEST(AdBlockBundledResourcesTest, VendoredFileLoadsIntoTheEngine) {
  std::string resources_json;
  ASSERT_TRUE(
      base::ReadFileToString(VendoredResourcesPath(), &resources_json));
  ASSERT_FALSE(resources_json.empty());

  auto storage = adblock::new_resource_storage(resources_json);

  // Named rather than counted: a count would pass on a file that parsed into
  // the wrong thing. These two are what the shipped filter lists refer to.
  const std::string de_amp = "de-amp.js";
  const std::string brave_fix = "brave-fix.js";
  EXPECT_TRUE(adblock::has_resource_for_testing(*storage, de_amp));
  EXPECT_TRUE(adblock::has_resource_for_testing(*storage, brave_fix));
}

TEST(AdBlockBundledResourcesTest, EmptyStorageDoesNotHaveThem) {
  // The state this bundling replaced: with no component and no bundled copy,
  // the provider handed the engine an empty storage. If this ever passes the
  // same assertions as the test above, the test above is proving nothing.
  auto empty = adblock::new_empty_resource_storage();
  const std::string de_amp = "de-amp.js";
  EXPECT_FALSE(adblock::has_resource_for_testing(*empty, de_amp));
}

}  // namespace brave_shields
