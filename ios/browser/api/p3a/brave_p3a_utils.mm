/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/p3a/brave_p3a_utils.h"

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/p3a/buildflags/buildflags.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/pref_names.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_P3A)  // Growser-289
#include "brave/components/p3a/p3a_service.h"
#endif

P3AMetricLogType const P3AMetricLogTypeSlow =
    static_cast<P3AMetricLogType>(p3a::MetricLogType::kSlow);
P3AMetricLogType const P3AMetricLogTypeTypical =
    static_cast<P3AMetricLogType>(p3a::MetricLogType::kTypical);
P3AMetricLogType const P3AMetricLogTypeExpress =
    static_cast<P3AMetricLogType>(p3a::MetricLogType::kExpress);

NSString* const P3ACreativeMetricPrefix =
    base::SysUTF8ToNSString(p3a::kCreativeMetricPrefix);

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation P3ACallbackRegistration {
  base::CallbackListSubscription _sub;
}
- (instancetype)initWithSubscription:(base::CallbackListSubscription)sub {
  if ((self = [super init])) {
    _sub = std::move(sub);
  }
  return self;
}
@end

@implementation BraveP3AUtils {
  raw_ptr<PrefService> _localState;
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  scoped_refptr<p3a::P3AService> _p3aService;
#endif
}

#if BUILDFLAG(ENABLE_P3A)  // Growser-289
- (instancetype)initWithLocalState:(PrefService*)localState
                        p3aService:(scoped_refptr<p3a::P3AService>)p3aService {
  if ((self = [super init])) {
    _localState = localState;
    _p3aService = p3aService;
  }
  return self;
}
#else
// Growser-289: no engine to hold. Every method below that needs one answers
// the way it already did for a missing service: no registration, no-op.
- (instancetype)initWithLocalState:(PrefService*)localState {
  if ((self = [super init])) {
    _localState = localState;
  }
  return self;
}
#endif

- (bool)isP3AEnabled {
  return _localState->GetBoolean(p3a::kP3AEnabled);
}

- (void)setIsP3AEnabled:(bool)isP3AEnabled {
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  _localState->SetBoolean(p3a::kP3AEnabled, isP3AEnabled);
  _localState->CommitPendingWrite();
#endif
}

- (bool)isP3APreferenceManaged {
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  return _localState->IsManagedPreference(p3a::kP3AEnabled);
#else
  // Growser-289: decided at build time, which is what "managed" means to the
  // UI - every P3A toggle (onboarding, settings) hides itself for a managed
  // pref, and the setter above cannot turn an engine that is not there on.
  return true;
#endif
}

- (bool)isNoticeAcknowledged {
  return _localState->GetBoolean(p3a::kP3ANoticeAcknowledged);
}

- (void)setIsNoticeAcknowledged:(bool)isNoticeAcknowledged {
  _localState->SetBoolean(p3a::kP3ANoticeAcknowledged, isNoticeAcknowledged);
  _localState->CommitPendingWrite();
}

- (P3ACallbackRegistration*)registerRotationCallback:
    (void (^)(P3AMetricLogType logType))callback {
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  if (!_p3aService) {
    return nil;
  }
  return [[P3ACallbackRegistration alloc]
      initWithSubscription:_p3aService->RegisterRotationCallback(
                               base::BindRepeating(
                                   ^(p3a::MetricLogType log_type) {
                                     callback(static_cast<P3AMetricLogType>(
                                         log_type));
                                   }))];
#else
  return nil;
#endif
}

- (P3ACallbackRegistration*)registerMetricCycledCallback:
    (void (^)(NSString* histogramName))callback {
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  if (!_p3aService) {
    return nil;
  }
  return [[P3ACallbackRegistration alloc]
      initWithSubscription:_p3aService->RegisterMetricCycledCallback(
                               base::BindRepeating(
                                   ^(const std::string& histogram_name) {
                                     callback(base::SysUTF8ToNSString(
                                         histogram_name));
                                   }))];
#else
  return nil;
#endif
}

- (void)registerDynamicMetric:(NSString*)histogramName
                      logType:(P3AMetricLogType)logType {
  [self registerDynamicMetric:histogramName
                      logType:logType
              mainThreadBound:YES];
}

- (void)registerDynamicMetric:(NSString*)histogramName
                      logType:(P3AMetricLogType)logType
              mainThreadBound:(BOOL)mainThreadBound {
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  if (!_p3aService) {
    return;
  }
  _p3aService->RegisterDynamicMetric(base::SysNSStringToUTF8(histogramName),
                                     static_cast<p3a::MetricLogType>(logType),
                                     mainThreadBound);
#endif
}

- (void)removeDynamicMetric:(NSString*)histogramName {
#if BUILDFLAG(ENABLE_P3A)  // Growser-289
  if (!_p3aService) {
    return;
  }
  _p3aService->RemoveDynamicMetric(base::SysNSStringToUTF8(histogramName));
#endif
}

void UmaHistogramExactLinear(NSString* name,
                             NSInteger sample,
                             NSInteger exclusive_max) {
  base::UmaHistogramExactLinear(base::SysNSStringToUTF8(name), sample,
                                exclusive_max);
}

void UmaHistogramBoolean(NSString* name, bool sample) {
  base::UmaHistogramBoolean(base::SysNSStringToUTF8(name), sample);
}

void UmaHistogramPercentage(NSString* name, NSInteger percent) {
  base::UmaHistogramPercentage(base::SysNSStringToUTF8(name), percent);
}

void UmaHistogramCustomCounts(NSString* name,
                              NSInteger sample,
                              NSInteger min,
                              NSInteger exclusive_max,
                              size_t buckets) {
  base::UmaHistogramCustomCounts(base::SysNSStringToUTF8(name), sample, min,
                                 exclusive_max, buckets);
}

void UmaHistogramCustomTimes(NSString* name,
                             NSTimeInterval sample,
                             NSTimeInterval min,
                             NSTimeInterval max,
                             size_t buckets) {
  base::UmaHistogramCustomTimes(base::SysNSStringToUTF8(name),
                                base::Seconds(sample), base::Seconds(min),
                                base::Seconds(max), buckets);
}

void UmaHistogramTimes(NSString* name, NSTimeInterval sample) {
  base::UmaHistogramTimes(base::SysNSStringToUTF8(name), base::Seconds(sample));
}

void UmaHistogramMediumTimes(NSString* name, NSTimeInterval sample) {
  base::UmaHistogramMediumTimes(base::SysNSStringToUTF8(name),
                                base::Seconds(sample));
}

void UmaHistogramLongTimes(NSString* name, NSTimeInterval sample) {
  base::UmaHistogramLongTimes(base::SysNSStringToUTF8(name),
                              base::Seconds(sample));
}

void UmaHistogramMemoryKB(NSString* name, NSInteger sample) {
  base::UmaHistogramMemoryKB(base::SysNSStringToUTF8(name), sample);
}

void UmaHistogramMemoryMB(NSString* name, NSInteger sample) {
  base::UmaHistogramMemoryMB(base::SysNSStringToUTF8(name), sample);
}

void UmaHistogramMemoryLargeMB(NSString* name, NSInteger sample) {
  base::UmaHistogramMemoryLargeMB(base::SysNSStringToUTF8(name), sample);
}

@end
