// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveStrings
import BraveUI
import Foundation
import Preferences
import SwiftUI

struct NewTabPageSettingsView: View {
  var isSponsoredBackgroundsSupported: Bool
  var linkTapped: ((URLRequest) -> Void)?

  @ObservedObject private var backgroundImages = Preferences.NewTabPage.backgroundImages
  @ObservedObject private var showNewTabPrivacyHub = Preferences.NewTabPage.showNewTabPrivacyHub
  @ObservedObject private var topSitesModeSelection = Preferences.NewTabPage.topSitesMode

  // Growser-290: ads are compiled out, so there is no pref behind this.
  @State private var isSponsoredAdsEnabled = false

  init(
    isSponsoredBackgroundsSupported: Bool,
    linkTapped: ((URLRequest) -> Void)?,
    prefs: any PrefService
  ) {
    self.isSponsoredBackgroundsSupported = isSponsoredBackgroundsSupported
    self.linkTapped = linkTapped
  }

  var body: some View {
    Form {
      Section {
        Toggle(Strings.NTP.settingsBackgroundImages, isOn: $backgroundImages.value)
        if backgroundImages.value, isSponsoredBackgroundsSupported {
          NavigationLink {
            BackgroundMediaTypePicker(
              isSponsoredAdsEnabled: $isSponsoredAdsEnabled
            )
            .environment(
              \.openURL,
              OpenURLAction { _ in
                self.linkTapped?(URLRequest(url: .brave.newTabTakeoverLearnMoreLinkUrl))
                return .handled
              }
            )
          } label: {
            LabeledContent {
              if isSponsoredAdsEnabled {
                Text(Strings.NTP.settingsSponsoredImagesSelection)
              } else {
                Text(Strings.NTP.settingsDefaultImagesOnly)
              }
            } label: {
              Text(Strings.NTP.settingsBackgroundImageSubMenu)
            }
          }
        }
      } header: {
        Text(Strings.NTP.settingsBackgroundImages)
      }
      Section {
        // Growser-310: no privacy stats card on the new tab page to show or hide.
        if FeatureList.kTopsitesEnabled.enabled {
          Picker(
            Strings.NTP.topsites,
            selection: $topSitesModeSelection.value
          ) {
            ForEach(TopSitesMode.allCases) { mode in
              Text(mode.title)
            }
          }
          .tint(Color(braveSystemName: .textTertiary))
        } else {
          Toggle(
            Strings.Widgets.favoritesWidgetTitle,
            isOn: $topSitesModeSelection.value.isEnabled
          )
        }
      } header: {
        Text(Strings.Widgets.widgetTitle)
      }
    }
    .tint(Color(braveSystemName: .primary40))
    .navigationTitle(Strings.NTP.settingsTitle)
    .navigationBarTitleDisplayMode(.inline)
  }

  private struct BackgroundMediaTypePicker: View {
    @Binding var isSponsoredAdsEnabled: Bool

    @Environment(\.dismiss) private var dismiss

    var body: some View {
      Form {
        Section {
          Picker("", selection: $isSponsoredAdsEnabled) {
            Text(Strings.NTP.settingsDefaultImagesOnly)
              .tag(false)
            Text(Strings.NTP.settingsSponsoredImagesSelection)
              .tag(true)
          }
          .pickerStyle(.inline)
          .labelsHidden()
          .onChange(of: isSponsoredAdsEnabled, initial: false) {
            dismiss()
          }
        } header: {
          Text(Strings.NTP.settingsBackgroundImageSubMenu)
        } footer: {
          // Contains markdown with a link
          Text(LocalizedStringKey(Strings.NTP.imageTypeSelectionDescription))
            .tint(Color(braveSystemName: .textInteractive))
        }
      }
      .navigationTitle(Strings.NTP.settingsBackgroundImageSubMenu)
      .navigationBarTitleDisplayMode(.inline)
    }
  }
}

class NTPTableViewController: UIHostingController<NewTabPageSettingsView> {
  // Growser-290: no rewards, so sponsored backgrounds are never supported.
  var linkTapped: ((URLRequest) -> Void)?

  init(linkTapped: ((URLRequest) -> Void)?, prefs: any PrefService) {
    super.init(
      rootView: .init(
        isSponsoredBackgroundsSupported: false,
        linkTapped: linkTapped,
        prefs: prefs
      )
    )
  }

  @available(*, unavailable)
  required init(coder: NSCoder) {
    fatalError()
  }
}
