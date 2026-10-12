/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_CONSTANTS_H_
#define BRAVE_COMPONENTS_TOR_CONSTANTS_H_

#include <cstdint>
#include <iterator>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/safe_base_name.h"
#include "build/build_config.h"
#include "crypto/sha2.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
// Growser-157: our own component. Brave's server answers a fork 403,
// so the client is published by us - see scripts/make-tor-component.py
// and deploy/growser-backend. The id, magogemiibiafhlbpenemlaomhplhiok, is the
// first 128 bits of the key hash below.
inline constexpr char kTorClientComponentName[] =
    "Growser Tor Client Updater (Windows)";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0xc0, 0x6e, 0x64, 0xc8, 0x81, 0x80, 0x57, 0xb1, 0xf4, 0xd4, 0xcb,
    0x0e, 0xc7, 0xfb, 0x78, 0xea, 0xdc, 0xc8, 0x07, 0x05, 0x5b, 0xdc,
    0x01, 0xf8, 0xea, 0x41, 0x41, 0x3e, 0xde, 0xa6, 0x98, 0xbb};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_MAC)
// Growser-163: our own component, with a key of its own - the Brave id that
// stood here names a package we do not publish and the Worker does not serve.
// A pair per platform is what upstream does and what we need: sharing the
// Windows id would give two platforms the same appid for different bytes, and
// the Worker filters by appid.
inline constexpr char kTorClientComponentName[] =
    "Growser Tor Client Updater (macOS)";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0x46, 0xf4, 0x67, 0xa3, 0xb9, 0x1b, 0x35, 0xc4, 0x12, 0x1f, 0x24,
    0x84, 0x6e, 0x90, 0xcd, 0x02, 0x78, 0x5a, 0x1f, 0x72, 0x89, 0x7e,
    0x1f, 0x94, 0x21, 0x1f, 0xeb, 0xa0, 0x13, 0x65, 0x34, 0xd8};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#elif BUILDFLAG(IS_LINUX)
// Growser-202: ours, the third and last platform, for exactly the reason the
// other two are. What stood here was Brave's id - a package we do not publish
// and our Worker does not serve - so Tor on Linux could only ever have asked
// about somebody else's component and downloaded nothing. Key:
// ../growser-keys/tor_client_component_linux.pem.
//
// One pair, not two: we build and publish x86_64 only. An arm64 Linux would
// need its own key, its own package and its own row in the Worker, and the id
// below would be wrong for it - wrong here means fetching a binary built for
// another machine, so it is a build error rather than a silent one.
#if !defined(ARCH_CPU_X86_64)
#error "Growser-202: the Linux Tor component is published for x86_64 only"
#endif
inline constexpr char kTorClientComponentName[] =
    "Growser Tor Client Updater (Linux)";
inline constexpr uint8_t kTorClientComponentPublicKeySHA256[32] = {
    0x23, 0x81, 0xc9, 0x4e, 0x9e, 0xcc, 0x66, 0xec, 0x74, 0x9a, 0x61,
    0x53, 0xcd, 0xc9, 0x7f, 0x1f, 0xc0, 0x8f, 0x5f, 0x5d, 0xe6, 0x86,
    0xe5, 0x00, 0x5b, 0x7e, 0x99, 0x65, 0x59, 0x94, 0x27, 0xc4};
static_assert(std::size(kTorClientComponentPublicKeySHA256) ==
              crypto::kSHA256Length);
#endif

// Returns the path for for where the Tor client binary is installed.
base::FilePath GetTorClientDirectory();

// Returns the path client execution path, based on the installation path for
// components, the `install_dir` provided, and the `filename`.
base::FilePath GetClientExecutablePath(const base::SafeBaseName& install_dir,
                                       const base::SafeBaseName& executable);

// Returns the path for the torrc file, based on the installation path for
// components, and the `install_dir` provided.
base::FilePath GetTorRcPath(const base::SafeBaseName& install_dir);

// Returns the path for the client's `--DataDirectory` argument.
base::FilePath GetTorDataPath();

// Return the directory path for the watcher arguments passed to the client.
base::FilePath GetTorWatchPath();

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_CONSTANTS_H_
