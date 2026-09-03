/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_CONSTANTS_H_
#define BRAVE_COMPONENTS_TOR_CONSTANTS_H_

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/files/safe_base_name.h"
#include "build/build_config.h"

namespace tor {

#if BUILDFLAG(IS_WIN)
// Growser-157: our own component. Brave's server answers a fork 403,
// so the client is published by us - see scripts/make-tor-component.py
// and deploy/growser-backend. The id is the first 128 bits of the key's
// SHA-256, so these two lines cannot disagree without the updater
// refusing whatever it downloads. Windows only for now: no macOS or
// Linux package exists yet, and Tor stays off there.
inline constexpr char kTorClientComponentName[] =
    "Growser Tor Client Updater (Windows)";
inline constexpr char kTorClientComponentId[] =
    "magogemiibiafhlbpenemlaomhplhiok";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsU43JFRgCUOdCDiDksl0"
    "d6gLxxhcqij3Oa4PPvCvDniiheV8pzgglEjvzaTrcL9rZTND+3Mefo10jVo4MlRm"
    "9pvrqtiRbU9eBUcOzrM75XPvadw8s09LUXcXvIOw/ywSqm7uPAD11iICQCiKNtAz"
    "C+5WXyrCvvZ36+U5YA1ZTilDP2Mjlx8gWrli7J5URjkV1BCGaDjfd0M+xUAXuKPn"
    "wp4je9D7bn+gET/jkxgPlG19DIW7MIcRR+b+wQIMN+5zkyWUNlrNxOJmO9Pfbn4n"
    "Afw2HcTEpwaDhn3Q7Ns7lEZ0WjM+oCyjEvLwE1jm4R9cZJXCuzDn5UWvMrfxJ4Ku"
    "rwIDAQAB";
#elif BUILDFLAG(IS_MAC)
// Growser-163: our own component, with a key of its own - the Brave id that
// stood here names a package we do not publish and the Worker does not serve.
// A pair per platform is what upstream does and what we need: sharing the
// Windows id would give two platforms the same appid for different bytes, and
// the Worker filters by appid.
inline constexpr char kTorClientComponentName[] =
    "Growser Tor Client Updater (macOS)";
inline constexpr char kTorClientComponentId[] =
    "egpeghkdljbldfmebcbpceiegojamnac";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAh1eNm711Om6MWVbJJ+06"
    "qcd05WCdLFdvPSA/r8NesxFQNXYSki0PPlK3s16AL6ndkYaI6/yLRPpm8nsgbSf6"
    "TC9Vt/fyyHIS6CziWR+RPF+ZIq1Yd2gkJpsP6GHgUnuG0Vv4u1OwqoVzyxUpFwKJ"
    "wCs8Ud8uoRqF7v3y8PtgYp8mbfJyGaDo4MupGFJ2dnslP6Pd4bQ+izpqBoyQh0/7"
    "WPMlLDwHd8c6wxUXt9V2ZnlIUE6NMGD8BjstL4m9wmGRdK9WPxDJNF2QqzAeSeqf"
    "SFvyl97MZmeBvYWXCiIkY3x5RUTp/4Uo84RBXkzP9hrigvhMpR+tfFStwvHKkvQ/"
    "TQIDAQAB";
#elif BUILDFLAG(IS_LINUX)
inline constexpr char kTorClientComponentName[] =
    "Brave Tor Client Updater (Linux)";
#if defined(ARCH_CPU_ARM64)
inline constexpr char kTorClientComponentId[] =
    "monolafkoghdlanndjfeebmdfkbklejg";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzqb14fggDpbjZtv3HKmR"
    "UTnvfDTcqVbVZo0DdCHQi6SwxDlRweGwsvsHuy9U37VBr41ha/neemQGf+5qkWgY"
    "y+mzzAkb5ZtrHkBSOOsZdyO9WEj7GwXuAx9FvcxG2zPpA/CvagnC14VhMyUFLL8v"
    "XdfHYPmQOtIVdW3eR0G/4JP/mTbnAEkipQfxrDMtDVpX+FDB+Zy5yEMGKWHRLcdH"
    "bHUgb/VhB9ppt0LKRjM44KSpyPDlYquXNcn3WFmxHoVm7PZ3LTAn3eSNZrT4ptmo"
    "KveT4LgWtObrHoZtrg+/LnHAi1GYf8PHrRc+o/FptobOWoUN5lt8NvhLjv85ERBt"
    "rQIDAQAB";
#else  // #if defined(ARCH_CPU_ARM64)
inline constexpr char kTorClientComponentId[] =
    "biahpgbdmdkfgndcmfiipgcebobojjkp";
inline constexpr char kTorClientComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAseuq8dXKawkZC7RSE7xb"
    "lRwh6DD+oPEGEjZWKh596/42IrWNQw60gRIR6s7x0YHh5geFnBRkx9bisEXOrFkq"
    "oArVY7eD0gMkjpor9CneD5CnCxc9/2uIPajtXfAmmLAHtN6Wk7yW30SkRf/WvLWX"
    "/H+PqskQBN7I5MO7sveYxSrRMSj7prrFHEiFmXTgG/DwjpzrA7KV6vmzz/ReD51o"
    "+UuLHE7cxPhnsNd/52uY3Lod3GhxvDoXKYx9kWlzBjxB53A2eLBCDIwwCpqS4/Ib"
    "RSJhvF33KQT8YM+7V1MitwB49klP4aEWPXwOlFHmn9Dkmlx2RbO7S0tRcH9UH4LK"
    "2QIDAQAB";
#endif
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
