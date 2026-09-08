/* Copyright (c) 2026 Dmitry Golubnichiy. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEBHARVESTER_WEBHARVESTER_H_
#define BRAVE_COMPONENTS_WEBHARVESTER_WEBHARVESTER_H_

// growser (#212): webharvester ships inside the browser on every platform and
// is DISABLED by default.
//
// The default is a security property rather than a preference. The extension
// declares 62 permissions - among them debugger, nativeMessaging, proxy,
// management, cookies and history - plus <all_urls>, and a component extension
// receives everything it declares with no prompt and cannot be removed by the
// user the way an installed extension can. Shipping it enabled would hand
// every user a browser in which one extension can attach a debugger to any
// page, read every cookie and speak to native binaries.
//
// So the bytes travel with the browser and the decision to run them does not.

namespace webharvester {

// Fixed by the "key" in the extension's own manifest.json, so it is the same
// in every build and on every platform. Recomputed from that key rather than
// copied: sha256 of the DER public key, first 16 bytes, each nibble mapped
// 0-15 to a-p.
inline constexpr char kExtensionId[] = "epejhconcffjigpklgfljmdfbmhodacj";

// Off by default. Registered in brave_profile_prefs.cc; the user flips it from
// settings and Claude Code flips it the same way, because a pref is the one
// place both can reach.
inline constexpr char kEnabledPref[] = "growser.webharvester_enabled";

// The extension's root as the resource manager serves it. Must equal the
// resource_path prefix in components/webharvester/resources.grd - the loader
// asks for files under this name and grit answers under that one, and nothing
// but this comment ties the two together.
inline constexpr char kExtensionRoot[] = "webharvester";

// Turns it on for one launch without touching the pref, for an agent that
// starts the browser itself. Deliberately does not persist: a session-scoped
// grant is easier to reason about than one that outlives the reason for it.
inline constexpr char kEnableSwitch[] = "enable-webharvester";

}  // namespace webharvester

#endif  // BRAVE_COMPONENTS_WEBHARVESTER_WEBHARVESTER_H_
