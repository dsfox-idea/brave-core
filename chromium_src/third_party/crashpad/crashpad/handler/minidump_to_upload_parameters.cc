/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/crashpad/crashpad/handler/minidump_to_upload_parameters.h"

#include <string_view>

#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"

#define BreakpadHTTPFormParametersFromMinidump \
  BreakpadHTTPFormParametersFromMinidump_ChromiumImpl
#include <third_party/crashpad/crashpad/handler/minidump_to_upload_parameters.cc>
#undef BreakpadHTTPFormParametersFromMinidump

namespace crashpad {

namespace {

// GlitchTip's minidump endpoint reads one form field, `sentry` (JSON), and
// copies its release and tags into the event; every other field Crashpad
// sends is dropped (growser#412). Its tag values are 255 characters at most.
constexpr char kSentryField[] = "sentry";
constexpr size_t kMaxTagBytes = 255;
constexpr const char* kTaggedKeys[] = {"ptype", "plat", "prod", "channel",
                                       "LOG_FATAL"};

std::string_view ValueOf(const std::map<std::string, std::string>& parameters,
                         const char* key) {
  auto it = parameters.find(key);
  return it == parameters.end() ? std::string_view() : it->second;
}

}  // namespace

std::map<std::string, std::string> BreakpadHTTPFormParametersFromMinidump(
    const ProcessSnapshot* process_snapshot) {
  std::map<std::string, std::string> parameters =
      BreakpadHTTPFormParametersFromMinidump_ChromiumImpl(process_snapshot);

  base::DictValue tags;
  for (const char* key : kTaggedKeys) {
    std::string_view value = ValueOf(parameters, key);
    if (!value.empty()) {
      tags.Set(key, base::TruncateUTF8ToByteSize(value, kMaxTagBytes));
    }
  }
  base::DictValue sentry;
  std::string_view version = ValueOf(parameters, "ver");
  if (!version.empty()) {
    sentry.Set("release", base::TruncateUTF8ToByteSize(version, kMaxTagBytes));
  }
  if (!tags.empty()) {
    sentry.Set("tags", std::move(tags));
  }
  if (sentry.empty()) {
    return parameters;
  }
  if (std::optional<std::string> json = base::WriteJson(sentry)) {
    InsertOrReplaceMapEntry(&parameters, kSentryField, *json);
  }
  return parameters;
}

}  // namespace crashpad
