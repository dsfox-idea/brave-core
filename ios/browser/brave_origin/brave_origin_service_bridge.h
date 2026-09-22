// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSString* BraveOriginPolicyKey NS_TYPED_EXTENSIBLE_ENUM;
// Growser-262: seven keys are gone with the policies they named - wallet, AI
// chat, rewards, talk, news, VPN and the stats ping. Each policy left
// brave_policies.gni when its feature was removed, so `policy::key::` stopped
// declaring the constant and this bridge could not compile: it is the second
// place iOS names those keys, after brave_simple_policy_map_ios.h, and the
// first build anyone ran here failed on both. The Swift side that bound them
// (Origin's settings screen) loses those rows with them.
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyP3AEnabled;
OBJC_EXPORT BraveOriginPolicyKey const BraveOriginPolicyKeyPlaylistEnabled;

NS_SWIFT_NAME(BraveOriginService)
@protocol BraveOriginServiceBridge

// Asynchronously check purchase state via SKU credential summary.
// The callback receives true if the user has a valid Origin purchase.
- (void)checkPurchaseState:(void (^)(BOOL isPurchased))completionHandler;

// Returns the cached purchase state (synchronous)
- (BOOL)isPurchased;

// Check if a policy is controlled by BraveOrigin
- (BOOL)isPolicyControlledByBraveOrigin:(BraveOriginPolicyKey)policyKey;

// Update the BraveOrigin policy value
- (BOOL)setPolicyValue:(BraveOriginPolicyKey)policyKey value:(BOOL)value;

// Get the current value of a BraveOrigin policy
// Returns nil if the policy value is not set
- (nullable NSNumber*)getPolicyValue:(BraveOriginPolicyKey)policyKey;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_H_
