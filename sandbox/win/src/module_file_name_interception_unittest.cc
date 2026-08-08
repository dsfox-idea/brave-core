/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/sandbox/win/src/module_file_name_interception.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <string>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "testing/gtest/include/gtest/gtest.h"

// growser (#64): these interceptions rewrite the executable's name in
// GetModuleFileName results so that callers see chrome.exe. Our executable is
// growser.exe, which is LONGER than chrome.exe, and upstream only ever handled
// the name growing (brave.exe -> chrome.exe): the length delta was an unsigned
// subtraction guarded by a static_assert forbidding anything else. The
// shrinking direction had no test at all, which is why nothing noticed that the
// shim matched no growser binary and silently did nothing.
//
// The Target* functions take the original API as a parameter, so they can be
// driven with a fake that yields any name - no sandbox and no real module.

namespace {

// The fake original API has no context parameter, so the name it should produce
// is passed through here.
std::wstring* g_wide_name = nullptr;
std::string* g_narrow_name = nullptr;

DWORD WINAPI FakeGetModuleFileNameW(HMODULE, LPWSTR buffer, DWORD size) {
  const std::wstring& name = *g_wide_name;
  const size_t copied = std::min<size_t>(name.size(), size);
  // SAFETY: this fake stands in for a Win32 out-parameter API. Its
  // contract is that `buffer` points at `size` characters, which is
  // exactly the span constructed here; nothing is written past it.
  auto out = UNSAFE_BUFFERS(base::span(buffer, static_cast<size_t>(size)));
  out.first(copied).copy_from(base::span(name).first(copied));
  if (copied < size) {
    out[copied] = 0;
  }
  return static_cast<DWORD>(copied);
}

DWORD WINAPI FakeGetModuleFileNameA(HMODULE, LPSTR buffer, DWORD size) {
  const std::string& name = *g_narrow_name;
  const size_t copied = std::min<size_t>(name.size(), size);
  // SAFETY: this fake stands in for a Win32 out-parameter API. Its
  // contract is that `buffer` points at `size` characters, which is
  // exactly the span constructed here; nothing is written past it.
  auto out = UNSAFE_BUFFERS(base::span(buffer, static_cast<size_t>(size)));
  out.first(copied).copy_from(base::span(name).first(copied));
  if (copied < size) {
    out[copied] = 0;
  }
  return static_cast<DWORD>(copied);
}

struct Result {
  DWORD returned;
  std::wstring value;
  DWORD last_error;
};

// Runs the wide interception over `input` with a buffer of `buffer_size`
// characters and reports what the caller would observe.
Result PatchWide(const std::wstring& input, size_t buffer_size) {
  std::wstring name = input;
  g_wide_name = &name;
  std::wstring buffer(buffer_size, L'\0');
  ::SetLastError(ERROR_SUCCESS);
  const DWORD returned = sandbox::TargetGetModuleFileNameW(
      &FakeGetModuleFileNameW, nullptr, buffer.data(),
      static_cast<DWORD>(buffer_size));
  const DWORD last_error = ::GetLastError();
  g_wide_name = nullptr;
  return {returned, std::wstring(buffer.data(), returned), last_error};
}

constexpr wchar_t kOurPath[] = L"C:\\Program Files\\Growser\\growser.exe";
constexpr wchar_t kOurPathPatched[] = L"C:\\Program Files\\Growser\\chrome.exe";

// The name shrinks: growser.exe (11) -> chrome.exe (10). This is the case
// upstream's static_assert rejected outright.
TEST(ModuleFileNameInterception, ShorterReplacement) {
  const Result r = PatchWide(kOurPath, 260);
  EXPECT_EQ(r.value, kOurPathPatched);
  EXPECT_EQ(r.returned, std::size(kOurPathPatched) - 1);
  // A name that got shorter cannot fail to fit in a buffer that already held
  // the longer one.
  EXPECT_NE(r.last_error, static_cast<DWORD>(ERROR_INSUFFICIENT_BUFFER));
}

// A buffer with exactly enough room for the original name plus its terminator
// is more than enough once the name shrinks.
TEST(ModuleFileNameInterception, ShorterReplacementExactBuffer) {
  const Result r = PatchWide(kOurPath, std::size(kOurPath));
  EXPECT_EQ(r.value, kOurPathPatched);
  EXPECT_NE(r.last_error, static_cast<DWORD>(ERROR_INSUFFICIENT_BUFFER));
}

// The growing direction still works: this is the specialisation used for test
// binaries, brave_browser_tests.exe (23) -> chrome_browser_tests.exe (24).
TEST(ModuleFileNameInterception, LongerReplacement) {
  const Result r = PatchWide(L"C:\\out\\brave_browser_tests.exe", 260);
  EXPECT_EQ(r.value, L"C:\\out\\chrome_browser_tests.exe");
}

// Growing into a buffer that cannot hold the result truncates and says so,
// which is what GetModuleFileName callers expect.
//
// The exact truncation is worth pinning because it is surprising, and it is
// upstream's behaviour rather than anything growser changed: the copy is capped
// at `dest_size - 1` to leave room for a terminator, so with one character too
// few the result loses TWO - one to the cap and one to the terminator strncpy_s
// writes. The returned length is still the pre-patch length, so a caller that
// trusts the return value over the terminator reads one character of padding.
TEST(ModuleFileNameInterception, LongerReplacementTruncates) {
  const std::wstring input = L"C:\\out\\brave_browser_tests.exe";
  const Result r = PatchWide(input, input.size() + 1);
  EXPECT_EQ(r.returned, static_cast<DWORD>(input.size()));
  EXPECT_EQ(r.last_error, static_cast<DWORD>(ERROR_INSUFFICIENT_BUFFER));
  EXPECT_EQ(r.value.substr(0, r.value.find(L'\0')),
            L"C:\\out\\chrome_browser_tests.e");
}

// Anything that is not one of our executables comes back untouched - notably
// brave.exe, which this shim used to match and no longer should.
TEST(ModuleFileNameInterception, UnrelatedNamesAreUntouched) {
  for (const wchar_t* name : {L"C:\\Program Files\\Brave\\brave.exe",
                              L"C:\\Windows\\explorer.exe",
                              L"C:\\out\\growser_helper.exe", L"growser.ex"}) {
    const Result r = PatchWide(name, 260);
    EXPECT_EQ(r.value, name);
  }
}

// The narrow (ANSI) interception is a separate specialisation, so a bug in one
// would not show up in the other.
TEST(ModuleFileNameInterception, NarrowShorterReplacement) {
  std::string name = "C:\\Program Files\\Growser\\growser.exe";
  g_narrow_name = &name;
  std::array<char, 260> buffer = {};
  const DWORD returned = sandbox::TargetGetModuleFileNameA(
      &FakeGetModuleFileNameA, nullptr, buffer.data(),
      static_cast<DWORD>(buffer.size()));
  g_narrow_name = nullptr;
  EXPECT_EQ(std::string(buffer.data(), returned),
            "C:\\Program Files\\Growser\\chrome.exe");
}

}  // namespace
