/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/crashpad/crashpad/handler/minidump_to_upload_parameters.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/crashpad/crashpad/snapshot/test/test_process_snapshot.h"

namespace crashpad {

using test::TestProcessSnapshot;

// growser#412: the upload carries a `sentry` field, the only one GlitchTip
// reads, holding the version, the process type and a CHECK's message.
TEST(GrowserMinidumpUploadParametersTest, CarriesASentryFieldForGlitchTip) {
  TestProcessSnapshot process_snapshot;
  process_snapshot.SetAnnotationsSimpleMap({
      {"ver", "157.26.1105.1"},
      {"ptype", "renderer"},
      {"plat", "Win64"},
      {"prod", "Growser"},
      {"LOG_FATAL", "feature_map.cc:44: " + std::string(400, 'x')},
  });

  std::map<std::string, std::string> parameters =
      BreakpadHTTPFormParametersFromMinidump(&process_snapshot);

  // Nothing Crashpad sent before is lost.
  EXPECT_EQ(parameters["ver"], "157.26.1105.1");
  EXPECT_EQ(parameters["ptype"], "renderer");

  std::optional<base::Value> sentry = base::JSONReader::Read(
      parameters["sentry"], base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(sentry && sentry->is_dict()) << parameters["sentry"];
  EXPECT_EQ(*sentry->GetDict().FindString("release"), "157.26.1105.1");
  const base::DictValue* tags = sentry->GetDict().FindDict("tags");
  ASSERT_TRUE(tags);
  EXPECT_EQ(*tags->FindString("ptype"), "renderer");
  EXPECT_EQ(*tags->FindString("plat"), "Win64");
  EXPECT_EQ(*tags->FindString("prod"), "Growser");
  const std::string* fatal = tags->FindString("LOG_FATAL");
  ASSERT_TRUE(fatal);
  EXPECT_EQ(fatal->size(), 255u);
  EXPECT_TRUE(fatal->starts_with("feature_map.cc:44: "));
  EXPECT_FALSE(tags->FindString("channel"));
}

TEST(GrowserMinidumpUploadParametersTest, AddsNothingWhenThereIsNothingToTell) {
  TestProcessSnapshot process_snapshot;
  process_snapshot.SetAnnotationsSimpleMap({{"guidance", "none"}});

  std::map<std::string, std::string> parameters =
      BreakpadHTTPFormParametersFromMinidump(&process_snapshot);

  EXPECT_EQ(parameters.count("sentry"), 0u);
  EXPECT_EQ(parameters["guidance"], "none");
}

}  // namespace crashpad
