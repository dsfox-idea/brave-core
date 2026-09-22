// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStore
import Combine
import SwiftUI

@propertyWrapper
struct OriginPolicyBooleanValue {
  var key: BraveOriginPolicyKey

  var wrappedValue: Bool {
    get { fatalError("Can only be used within OriginSettingsViewModel") }
    set { fatalError("Can only be used within OriginSettingsViewModel") }
  }

  static subscript(
    _enclosingInstance instance: OriginSettingsViewModel,
    wrapped wrappedKeyPath: ReferenceWritableKeyPath<OriginSettingsViewModel, Bool>,
    storage storageKeyPath: ReferenceWritableKeyPath<OriginSettingsViewModel, Self>
  ) -> Bool {
    get {
      instance.access(keyPath: wrappedKeyPath)
      let key = instance[keyPath: storageKeyPath].key
      return instance.service.getPolicyValue(key) as? Bool ?? false
    }
    set {
      let key = instance[keyPath: storageKeyPath].key
      let result = instance.withMutation(keyPath: wrappedKeyPath) {
        instance.service.setPolicyValue(key, value: newValue)
      }
      if result {
        withAnimation(.toast) {
          instance.isRestartToastVisible = true
        }
      }
    }
  }
}

@Observable
public class OriginSettingsViewModel {
  fileprivate let service: any BraveOriginService
  private let storeSDK: BraveStoreSDK
  private var productUpdateCancellable: AnyCancellable?

  public init(service: any BraveOriginService, storeSDK: BraveStoreSDK) {
    self.service = service
    self.storeSDK = storeSDK

    Task {
      await updatePurchaseStatus()
    }

    productUpdateCancellable = storeSDK.$originPurchaseProduct
      .sink { [weak self] _ in
        guard let self else { return }
        Task {
          await self.updatePurchaseStatus()
        }
      }
  }

  var isRestartToastVisible: Bool = false

  // Growser-262: seven properties are gone with the policies they bound to -
  // rewards, Leo, news, the statistics ping, talk, VPN and wallet. Each policy
  // left brave_policies.gni when its feature was removed, so the ObjC constant
  // this wrapper takes as its key stopped existing
  // (brave_origin_service_bridge.h). These two are the policies our list still
  // carries.
  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .p3AEnabled)
  var isP3AEnabled: Bool

  @ObservationIgnored
  @OriginPolicyBooleanValue(key: .playlistEnabled)
  var isPlaylistEnabled: Bool

  private(set) var isPuchaseLinkable: Bool = false

  @MainActor
  private func updatePurchaseStatus() async {
    guard let transaction = await storeSDK.originPurchaseProduct?.latestTransaction else {
      isPuchaseLinkable = false
      return
    }
    isPuchaseLinkable =
      switch transaction {
      case .verified:
        true
      case .unverified:
        false
      }
  }

  /// Reset all of the policy values back to their defaults
  func reset() {
    // Growser-262: the seven removed policies are not reset here because they
    // are not settable here any more - see the properties above.
    self.isP3AEnabled = false
    self.isPlaylistEnabled = false
  }
}
