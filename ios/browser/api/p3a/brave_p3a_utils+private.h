/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_PRIVATE_H_

#include "brave/components/p3a/buildflags/buildflags.h"
#include "brave/ios/browser/api/p3a/brave_p3a_utils.h"

class PrefService;

namespace p3a {
class P3AService;
}  // namespace p3a

NS_ASSUME_NONNULL_BEGIN

@interface BraveP3AUtils (Private)
#if BUILDFLAG(ENABLE_P3A)
- (instancetype)initWithLocalState:(PrefService*)localState
                        p3aService:(scoped_refptr<p3a::P3AService>)p3aService;
#else
// Growser-289: the P3A engine is compiled out, as on the desktop (#98).
- (instancetype)initWithLocalState:(PrefService*)localState;
#endif
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_PRIVATE_H_
