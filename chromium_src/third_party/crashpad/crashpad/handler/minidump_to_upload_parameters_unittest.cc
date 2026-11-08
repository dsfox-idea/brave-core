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
#include "third_party/crashpad/crashpad/snapshot/test/test_system_snapshot.h"

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

// growser#414, #415: an uncaught Objective-C exception and the OS build are
// what make an abort inside AppKit or Crashpad's iOS processor readable.
TEST(GrowserMinidumpUploadParametersTest, TellsTheExceptionAndTheOsBuild) {
  TestProcessSnapshot process_snapshot;
  process_snapshot.SetAnnotationsSimpleMap({
      {"lastexception", "NSInternalInconsistencyException reason boom"},
      {"exceptionName", "NSRangeException"},
  });
  auto system = std::make_unique<test::TestSystemSnapshot>();
  system->SetOSVersionFull("26.1 25B78");
  process_snapshot.SetSystem(std::move(system));

  std::map<std::string, std::string> parameters =
      BreakpadHTTPFormParametersFromMinidump(&process_snapshot);

  std::optional<base::Value> sentry = base::JSONReader::Read(
      parameters["sentry"], base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  ASSERT_TRUE(sentry && sentry->is_dict()) << parameters["sentry"];
  const base::DictValue* tags = sentry->GetDict().FindDict("tags");
  ASSERT_TRUE(tags);
  auto tag = [tags](const char* key) {
    const std::string* value = tags->FindString(key);
    return value ? *value : std::string("<missing>");
  };
  EXPECT_EQ(tag("lastexception"),
            "NSInternalInconsistencyException reason boom");
  EXPECT_EQ(tag("exceptionName"), "NSRangeException");
  EXPECT_EQ(tag("os.build"), "26.1 25B78");
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
