/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/execution_context/navigator_base.h"

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/notreached.h"
#include "base/system/sys_info.h"
#include "brave/components/brave_shields/core/common/farbling_prng.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"

namespace blink::probe {

void ApplyBraveHardwareConcurrencyOverride(blink::ExecutionContext* context,
                                           unsigned int* hardware_concurrency) {
  static constexpr unsigned kFakeMinProcessors = 4;
  static constexpr unsigned kFakeMaxProcessors = 8;
  unsigned true_value =
      static_cast<unsigned>(base::SysInfo::NumberOfProcessors());
  if (true_value < kFakeMinProcessors) {
    *hardware_concurrency = true_value;
    return;
  }
  unsigned farbled_value = true_value;
  switch (brave::GetBraveFarblingLevelFor(
      context, ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY,
      BraveFarblingLevel::OFF)) {
    case BraveFarblingLevel::OFF: {
      break;
    }
    case BraveFarblingLevel::MAXIMUM: {
      true_value = kFakeMaxProcessors;
      // "Maximum" behavior is "balanced" behavior but with a fake maximum,
      // so fall through here.
      [[fallthrough]];
    }
    case BraveFarblingLevel::BALANCED: {
      // growser (#82): still farbled, but only to a value a machine could
      // actually have.
      //
      // The old line picked any integer in [2, true_value], which produced
      // counts like 29 - a number no CPU reports. Reporting an impossible core
      // count does not hide the visitor among Chrome users, it announces that
      // this browser rewrites the property, which is exactly the signal we are
      // trying not to send. Picking from the counts real machines have keeps
      // the per-origin variation and costs nothing.
      static constexpr unsigned kPlausibleProcessorCounts[] = {
          2, 4, 6, 8, 10, 12, 16, 20, 24, 32};
      brave_shields::FarblingPRNG prng =
          brave::BraveSessionCache::From(*context).MakePseudoRandomGenerator();
      unsigned candidates = 0;
      for (unsigned count : kPlausibleProcessorCounts) {
        if (count >= kFakeMinProcessors && count <= true_value) {
          candidates++;
        }
      }
      // true_value > 2 here, so at least the entry for 2 qualifies.
      CHECK_GT(candidates, 0u);
      // Walked rather than indexed: -Wunsafe-buffer-usage rejects subscripting
      // a raw array, and a second pass is clearer here than a span.
      unsigned pick = prng() % candidates;
      for (unsigned count : kPlausibleProcessorCounts) {
        if (count < kFakeMinProcessors || count > true_value) {
          continue;
        }
        if (pick == 0) {
          farbled_value = count;
          break;
        }
        pick--;
      }
      break;
    }
    default:
      NOTREACHED();
  }
  *hardware_concurrency = farbled_value;
}

}  // namespace blink::probe

#define userAgent userAgent_ChromiumImpl
#define ApplyHardwareConcurrencyOverride                        \
  ApplyBraveHardwareConcurrencyOverride(GetExecutionContext(),  \
                                        &hardware_concurrency); \
  probe::ApplyHardwareConcurrencyOverride

#include <third_party/blink/renderer/core/execution_context/navigator_base.cc>
#undef ApplyHardwareConcurrencyOverride
#undef userAgent

namespace blink {

String NavigatorBase::userAgent() const {
  if (ExecutionContext* context = GetExecutionContext()) {
    if (!brave::AllowFingerprinting(
            context, ContentSettingsType::BRAVE_WEBCOMPAT_USER_AGENT)) {
      return brave::BraveSessionCache::From(*context).FarbledUserAgent(
          context->UserAgent());
    }
  }

  return userAgent_ChromiumImpl();
}

}  // namespace blink
