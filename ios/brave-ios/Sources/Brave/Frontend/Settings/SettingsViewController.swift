// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Growser-279: no AIChat.
import BraveCore
// Growser-281: no BraveNews.
import BraveShared
// Growser-283: no BraveStore.
import BraveUI
// Growser-280: no BraveVPN.
// Growser-287: no BraveWallet.
import Combine
import Data
import DataImporter
import Growth
import LocalAuthentication
import NetworkExtension
import Onboarding
// Growser-283: no Origin.
// Growser-282: no Playlist.
import Preferences
import Shared
import Static
import SwiftUI
import UIKit
import UserAgent
import Web
import WebKit

extension TabBarVisibility: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .always: return Strings.alwaysShow
    case .landscapeOnly: return Strings.showInLandscapeOnly
    case .never: return Strings.neverShow
    }
  }
}

extension Preferences.AutoCloseTabsOption: RepresentableOptionType {
  public var displayString: String {
    switch self {
    case .manually: return Strings.Settings.autocloseTabsManualOption
    case .oneDay: return Strings.Settings.autocloseTabsOneDayOption
    case .oneWeek: return Strings.Settings.autocloseTabsOneWeekOption
    case .oneMonth: return Strings.Settings.autocloseTabsOneMonthOption
    }
  }
}

protocol SettingsDelegate: AnyObject {
  func settingsOpenURLInNewTab(_ url: URL)
  func settingsOpenURLs(_ urls: [URL], loadImmediately: Bool)
  // Growser-283: no settingsDidCompleteOriginPurchase().

  func settingsCreateFakeTabs()
  func settingsCreateFakeBookmarks()
  func settingsCreateFakeHistory()
}

class SettingsViewController: TableViewController, BraveAccountDialogOpenerBridge {
  weak var settingsDelegate: SettingsDelegate?

  private let profile: LegacyBrowserProfile
  private let tabManager: TabManager
  // Growser-290: no rewards.
  // Growser-281: no feedDataSource.
  private let braveCore: BraveProfileController
  private let historyAPI: BraveHistoryAPI
  private let passwordAPI: BravePasswordAPI
  private let syncAPI: BraveSyncAPI
  private let syncProfileServices: BraveSyncProfileServiceIOS
  private let p3aUtilities: BraveP3AUtils
  private let localState: any PrefService
  private let attributionManager: AttributionManager
  // Growser-287: no keyring or crypto store.
  private let windowProtection: WindowProtection?
  private let ipfsAPI: IpfsAPI
  // Growser-284: no AltIconsModel - the alternate icons are Brave lions.

  private let featureSectionUUID: UUID = .init()
  private let displaySectionUUID: UUID = .init()


  private var cancellables: Set<AnyCancellable> = []

  init(
    profile: LegacyBrowserProfile,
    tabManager: TabManager,
    // Growser-281: no feedDataSource.
    windowProtection: WindowProtection?,
    p3aUtils: BraveP3AUtils,
    braveCore: BraveProfileController,
    localState: any PrefService,
    attributionManager: AttributionManager
  ) {
    self.profile = profile
    self.tabManager = tabManager
    self.windowProtection = windowProtection
    self.braveCore = braveCore
    self.localState = localState
    self.historyAPI = braveCore.historyAPI
    self.passwordAPI = braveCore.passwordAPI
    self.syncAPI = braveCore.syncAPI
    self.syncProfileServices = braveCore.syncProfileService
    self.p3aUtilities = p3aUtils
    self.attributionManager = attributionManager
    self.ipfsAPI = braveCore.ipfsAPI

    super.init(style: .insetGrouped)

    Task { @MainActor in
      await BraveOriginServiceFactory.get(profile: braveCore.profile)?.checkPurchaseState()
    }

    UIImageView.appearance(whenContainedInInstancesOf: [SettingsViewController.self]).tintColor =
      .label
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  @available(*, unavailable)
  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    navigationItem.title = Strings.settings
    tableView.accessibilityIdentifier = "SettingsViewController.tableView"

    tableView.separatorInset = UIEdgeInsets(top: 0, left: 16, bottom: 0, right: 0)

    view.backgroundColor = .systemGroupedBackground
    view.tintColor = UIColor(braveSystemName: .textInteractive)
    navigationController?.view.backgroundColor = .systemGroupedBackground

    setUpSections()

    // Growser-280: no NEVPNStatusDidChange observer.

    // Growser-284: no app icon row to refresh.
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    // Reset dev options access count
    aboutHeaderTapCount = 0
    navigationController?.setToolbarHidden(true, animated: animated)
  }

  // Growser-290: no displayRewardsDebugMenu().

  // Growser-281: no displayBraveNewsDebugMenu().

  private func displayBraveSearchDebugMenu() {
    let hostingController =
      UIHostingController(rootView: BraveSearchDebugMenu(logging: BraveSearchLogEntry.shared))

    navigationController?.pushViewController(hostingController, animated: true)
  }

  // Growser-287: no displayBraveWalletDebugMenu().

  // Growser-280: no vpnConfigChanged(notification:).

  // Do not use `sections` directly to access sections/rows. Use DataSource.sections instead.
  private func makeSections() -> [Static.Section] {
    var list = [
      defaultBrowserSection,
      makeFeaturesSection(),
      generalSection,
      displaySection,
      tabsSection,
      autofillSection,
      supportSection,
      aboutSection,
    ]

    // Growser-280: no "enable Brave VPN" header section.

    // Always show debug section in local builds and show if previously shown
    if !AppConstants.isOfficialBuild || Preferences.Debug.developerOptionsEnabled.value {
      list.append(debugSection)
    }

    return list
  }

  // MARK: - Sections

  // Growser-280: no enableBraveVPNSection.

  private lazy var defaultBrowserSection: Static.Section = {
    var rows: [Row] = []

    if IsBraveAccountEnabled() {
      rows.append(
        Row(
          selection: { [unowned self] in
            openBraveAccountSettings()
          },
          cellClass: BraveAccountIconCell.self
        )
      )
    }

    rows.append(
      contentsOf: [
        Row(
          text: Strings.setDefaultBrowserSettingsCell,
          selection: { [unowned self] in
            guard let settingsUrl = URL(string: UIApplication.openSettingsURLString) else {
              return
            }
            Task {
              if let windowScene = viewIfLoaded?.window?.windowScene {
                await DefaultBrowserPictureInPictureController.present(in: windowScene)
              }
              await UIApplication.shared.open(settingsUrl)
            }
          },
          image: UIImage(braveSystemNamed: "leo.set.as-default"),
          cellClass: MultilineButtonCell.self
        ),
        Row(
          text: Strings.addToDockSettingsCell,
          selection: { [weak self] in
            guard let self else { return }
            let controller = OnboardingController(
              environment: .init(
                p3aUtils: p3aUtilities,
                attributionManager: attributionManager,
                localState: localState
              ),
              steps: [.addToDock],
              showSplashScreen: false,
              showDismissButton: false
            ).then {
              $0.isModalInPresentation = true
              $0.modalPresentationStyle = .overFullScreen
            }
            self.present(controller, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.dock"),
          cellClass: MultilineButtonCell.self
        ),
        Row(
          text: Strings.importBrowsingDataSettingsMenuTitle,
          selection: { [unowned self] in
            let controller = UIHostingController(
              rootView: DataImportView(
                model: .init(
                  coordinator: SafariDataImporterCoordinatorImpl(profile: braveCore.profile)
                ),
                openURL: { [unowned self] url in
                  self.settingsDelegate?.settingsOpenURLInNewTab(url)
                  self.dismiss(animated: true)
                },
                dismiss: { [unowned self] in
                  self.navigationController?.popViewController(animated: true)
                },
                onDismiss: { [weak self] in
                  self?.navigationController?.setNavigationBarHidden(false, animated: false)
                }
              )
            )
            self.navigationController?.pushViewController(controller, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.import.arrow"),
          cellClass: MultilineButtonCell.self
        ),
      ]
    )

    return Static.Section(rows: rows)
  }()

  private func openBraveAccountWebUI(
    url: URL,
    dialogMode: BraveAccount.DialogMode? = nil
  ) {
    let controller = ChromeWebUIController(braveCore: braveCore, isPrivateBrowsing: false)
    let container = UINavigationController(rootViewController: controller)
    controller.title = L10nUtils.string(messageId: .BRAVE_ACCOUNT_TITLE)
    if let dialogMode {
      controller.webView.braveAccountDialogMode = dialogMode
    }
    controller.webView.braveAccountDialogOpener = self
    controller.webView.load(URLRequest(url: url))
    controller.navigationItem.rightBarButtonItem = .doneButton { [unowned container] in
      container.dismiss(animated: true)
    }

    // Present from whatever is on top rather than from here - UIKit ignores a
    // second presentation from a controller that is already presenting.
    var presenter: UIViewController = navigationController ?? self
    while let presented = presenter.presentedViewController {
      presenter = presented
    }
    presenter.present(container, animated: true)
  }

  private func openBraveAccountSettings() {
    openBraveAccountWebUI(
      // Growser-321: our WebUI scheme, not Brave's.
      url: URL(string: "\(URL.webUI.scheme)://account/\(BraveAccountSettingsPath)")!
    )
  }

  // MARK: - BraveAccountDialogOpenerBridge

  // Also called from the WebUI serving the account rows, which asks for the
  // dialog to be opened over it.
  func openBraveAccountDialog(
    initiatingServiceName: String = "",
    dialogMode: BraveAccount.DialogMode = .default
  ) {
    // Growser-321: our WebUI scheme, not Brave's.
    var components = URLComponents(string: "\(URL.webUI.scheme)://account")!
    if !initiatingServiceName.isEmpty {
      components.queryItems = [
        URLQueryItem(
          name: BraveAccountInitiatingServiceNameQueryParam,
          value: initiatingServiceName
        )
      ]
    }

    openBraveAccountWebUI(
      url: components.url!,
      dialogMode: dialogMode
    )
  }

  private func makeFeaturesSection() -> Static.Section {
    weak var spinner: SpinnerView?

    var section = Static.Section(
      header: .title(Strings.features),
      rows: [
        Row(
          text: Strings.braveShieldsAndPrivacySettingsTitle,
          selection: { [unowned self] in
            let controller = UIHostingController(
              rootView: AdvancedShieldsSettingsView(
                settings: AdvancedShieldsSettings(
                  profile: self.profile,
                  tabManager: self.tabManager,
                  // Growser-281: no feedDataSource.
                  debounceService: DebounceServiceFactory.get(privateMode: false),
                  braveShieldsSettings: BraveShieldsSettingsServiceFactory.get(
                    profile: braveCore.profile
                  ),
                  braveCore: braveCore,
                  p3aUtils: p3aUtilities,
                  localState: localState,
                  // Growser-290: no rewards.
                  braveStats: braveCore.braveStats,
                  webcompatReporterHandler: WebcompatReporter.ServiceFactory.get(
                    privateMode: false
                  ),
                  clearDataCallback: { [weak self] isLoading, isHistoryCleared in
                    guard let view = self?.navigationController?.view, view.window != nil else {
                      assertionFailure()
                      return
                    }

                    if isLoading, spinner == nil {
                      let newSpinner = SpinnerView()
                      newSpinner.present(on: view)
                      spinner = newSpinner
                    } else {
                      spinner?.dismiss()
                      spinner = nil
                    }

                    if isHistoryCleared {
                      // Donate Clear Browser History for suggestions
                      let clearBrowserHistoryActivity = ActivityShortcutManager.shared
                        .createShortcutActivity(type: .clearBrowsingHistory)
                      self?.userActivity = clearBrowserHistoryActivity
                      clearBrowserHistoryActivity.becomeCurrent()
                    }
                  }
                )
              )
            )

            controller.rootView.openURLAction = { [unowned self] url in
              self.settingsDelegate?.settingsOpenURLInNewTab(url)
              self.dismiss(animated: true)
            }

            self.navigationController?.pushViewController(controller, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.shield.done"),
          accessory: .disclosureIndicator
        )
      ],
      uuid: featureSectionUUID.uuidString
    )

    // Growser-290: no Brave Rewards settings row.

    // Growser-281: no Brave News settings row.

    // Growser-279: no Leo settings row.

    // Growser-280: no VPN settings row.

    // Growser-282: no Playlist settings row.

    if FeatureList.kBraveTranslateEnabled.enabled {
      section.rows.append(
        Row(
          text: Strings.BraveTranslate.settingsMenuTitle,
          selection: { [unowned self] in
            let translateSettings = UIHostingController(rootView: BraveTranslateSettingsView())
            self.navigationController?.pushViewController(translateSettings, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.product.translate"),
          accessory: .disclosureIndicator
        )
      )
    }

    return section
  }

  private lazy var generalSection: Static.Section = {
    var general = Static.Section(
      header: .title(Strings.settingsGeneralSectionTitle),
      rows: [
        Row(
          text: Strings.searchEngines,
          selection: { [unowned self] in
            let viewController = SearchSettingsViewController(profile: self.profile)
            self.navigationController?.pushViewController(viewController, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.search"),
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: Strings.Sync.syncTitle,
          selection: { [unowned self] in
            if syncAPI.isInSyncGroup {
              if Reachability.shared.status.connectionType == .offline {
                self.present(SyncAlerts.noConnection, animated: true)
                return
              }

              let syncSettingsViewController = SyncSettingsTableViewController(
                braveCoreMain: braveCore,
                windowProtection: windowProtection
              )

              self.navigationController?
                .pushViewController(syncSettingsViewController, animated: true)
            } else {
              let syncWelcomeViewController = SyncWelcomeViewController(
                braveCore: braveCore,
                windowProtection: windowProtection
              )

              self.navigationController?.pushViewController(
                syncWelcomeViewController,
                animated: true
              )
            }
          },
          image: UIImage(braveSystemNamed: "leo.product.sync"),
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        .boolRow(
          title: Strings.bookmarksLastVisitedFolderTitle,
          option: Preferences.General.showLastVisitedBookmarksFolder,
          image: UIImage(braveSystemNamed: "leo.folder.open")
        ),
        Row(
          text: Strings.Shortcuts.shortcutSettingsTitle,
          selection: { [unowned self] in
            self.navigationController?.pushViewController(
              ShortcutSettingsViewController(
                isPlaylistAvailable: false,  // Growser-282
                isBraveVPNAvailable: false,  // Growser-280
                isBraveNewsAvailable: false  // Growser-281
              ),
              animated: true
            )
          },
          image: UIImage(braveSystemNamed: "leo.siri.shorcut"),
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
      ]
    )

    // Growser-293: Sync is not offered until our own server exists (#43, #78).
    if !FeatureList.kBraveSync.enabled {
      general.rows.removeAll { $0.text == Strings.Sync.syncTitle }
    }

    let defaultHostContentSettings = braveCore.defaultHostContentSettings
    if UIDevice.isIpad {
      let defaultPageModeSwitch = SwitchAccessoryView(
        initialValue: defaultHostContentSettings.defaultPageMode == .desktop,
        valueChange: { value in
          defaultHostContentSettings.defaultPageMode = value ? .desktop : .mobile
        }
      )
      general.rows.append(
        Row(
          text: Strings.alwaysRequestDesktopSite,
          image: UIImage(braveSystemNamed: "leo.window.cursor"),
          accessory: .view(defaultPageModeSwitch),
          cellClass: MultilineSubtitleCell.self
        )
      )
    }

    let blockPopupsSwitch = SwitchAccessoryView(
      initialValue: !defaultHostContentSettings.popupsAllowed,
      valueChange: { value in
        defaultHostContentSettings.popupsAllowed = !value
      }
    )
    general.rows.append(contentsOf: [
      .boolRow(
        title: Strings.enablePullToRefresh,
        option: Preferences.General.enablePullToRefresh,
        image: UIImage(braveSystemNamed: "leo.browser.refresh")
      ),
      Row(
        text: Strings.blockPopups,
        image: UIImage(braveSystemNamed: "leo.shield.block"),
        accessory: .view(blockPopupsSwitch),
        cellClass: MultilineSubtitleCell.self
      ),
    ])

    let websiteRedirectsRow = Row(
      text: Strings.urlRedirectsSettings,
      selection: { [unowned self] in
        let controller = UIHostingController(rootView: WebsiteRedirectsSettingsView())
        self.navigationController?.pushViewController(controller, animated: true)
      },
      image: UIImage(braveSystemNamed: "leo.swap.horizontal"),
      accessory: .disclosureIndicator,
      cellClass: MultilineSubtitleCell.self
    )
    general.rows.append(websiteRedirectsRow)

    let browserLockRow = Row(
      text: Strings.Privacy.browserLock,
      detailText: Strings.Privacy.browserLockDescription,
      image: UIImage(braveSystemNamed: "leo.biometric.login"),
      accessory: .view(
        SwitchAccessoryView(
          initialValue: Preferences.Privacy.lockWithPasscode.value,
          valueChange: { [unowned self] isOn in
            if isOn {
              Preferences.Privacy.lockWithPasscode.value = isOn
            } else {
              self.askForLocalAuthentication { [weak self] success, error in
                if success {
                  Preferences.Privacy.lockWithPasscode.value = isOn
                }
              }
            }
          }
        )
      ),
      cellClass: MultilineSubtitleCell.self,
      uuid: Preferences.Privacy.lockWithPasscode.key
    )
    general.rows.append(browserLockRow)

    // Growser-283: no Brave Origin row.

    return general
  }()

  private lazy var tabsSection: Static.Section = {
    var tabs = Static.Section(header: .title(Strings.tabsSettingsSectionTitle), rows: [])

    if UIDevice.current.userInterfaceIdiom == .phone {
      tabs.rows.append(
        Row(cellClass: LocationViewPositionPickerCell.self)
      )
    }

    if UIDevice.current.userInterfaceIdiom == .pad {
      tabs.rows.append(
        Row(
          text: Strings.showTabsBar,
          image: UIImage(braveSystemNamed: "leo.window.tab"),
          accessory: .view(
            SwitchAccessoryView(
              initialValue: Preferences.General.tabBarVisibility.value
                != TabBarVisibility.never.rawValue,
              valueChange: {
                Preferences.General.tabBarVisibility.value =
                  $0 ? TabBarVisibility.always.rawValue : TabBarVisibility.never.rawValue
              }
            )
          ),
          cellClass: MultilineValue1Cell.self
        )
      )
    } else {
      var row = Row(
        text: Strings.showTabsBar,
        detailText: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value)?
          .displayString,
        image: UIImage(braveSystemNamed: "leo.window.tab"),
        accessory: .disclosureIndicator,
        cellClass: MultilineValue1Cell.self
      )
      row.selection = { [unowned self] in
        // Show options for tab bar visibility
        let optionsViewController = OptionSelectionViewController<TabBarVisibility>(
          options: TabBarVisibility.allCases,
          selectedOption: TabBarVisibility(rawValue: Preferences.General.tabBarVisibility.value),
          optionChanged: { _, option in
            Preferences.General.tabBarVisibility.value = option.rawValue
            self.dataSource.reloadCell(row: row, section: tabs, displayText: option.displayString)
          }
        )
        optionsViewController.headerText = Strings.showTabsBar
        optionsViewController.navigationItem.title = Strings.showTabsBar
        self.navigationController?.pushViewController(optionsViewController, animated: true)
      }
      tabs.rows.append(row)
    }

    let autoCloseSetting =
      Preferences
      .AutoCloseTabsOption(rawValue: Preferences.General.autocloseTabs.value)?.displayString
    var autoCloseTabsRow =
      Row(
        text: Strings.Settings.autocloseTabsSetting,
        detailText: autoCloseSetting,
        image: UIImage(braveSystemNamed: "leo.window.tabs"),
        accessory: .disclosureIndicator,
        cellClass: MultilineSubtitleCell.self
      )
    autoCloseTabsRow.selection = { [unowned self] in
      let optionsViewController = OptionSelectionViewController<Preferences.AutoCloseTabsOption>(
        options: Preferences.AutoCloseTabsOption.allCases,
        selectedOption:
          Preferences.AutoCloseTabsOption(rawValue: Preferences.General.autocloseTabs.value),
        optionChanged: { _, option in
          Preferences.General.autocloseTabs.value = option.rawValue
          self.dataSource.reloadCell(
            row: autoCloseTabsRow,
            section: tabs,
            displayText: option.displayString
          )
        }
      )
      optionsViewController.headerText = Strings.Settings.autocloseTabsSetting
      optionsViewController.footerText = Strings.Settings.autocloseTabsSettingFooter
      optionsViewController.navigationItem.title = Strings.Settings.autocloseTabsSetting
      self.navigationController?.pushViewController(optionsViewController, animated: true)
    }

    tabs.rows.append(autoCloseTabsRow)

    if !Preferences.Privacy.privateBrowsingOnly.value {
      let privateTabsRow = Row(
        text: Strings.TabsSettings.privateTabsSettingsTitle,
        selection: { [unowned self] in
          let vc = UIHostingController(
            rootView: PrivateTabsView(
              tabManager: tabManager,
              askForAuthentication: self.askForLocalAuthentication
            )
          )
          self.navigationController?.pushViewController(vc, animated: true)
        },
        image: UIImage(braveSystemNamed: "leo.product.private-window"),
        accessory: .disclosureIndicator
      )

      tabs.rows.append(privateTabsRow)
    }

    if FeatureList.kQuickViewEnabled.enabled {
      var quickViewRow = Row(
        text: Strings.TabsSettings.openLinkInQuickViewModeTitle,
        detailText: Strings.TabsSettings.openLinkInQuickViewModeDescription,
        image: UIImage(braveSystemNamed: "leo.browser.quick-view"),
        accessory: .view(
          SwitchAccessoryView(
            initialValue: Preferences.General.openLinkInQuickViewMode.value,
            valueChange: { newValue in
              Preferences.General.openLinkInQuickViewMode.value = newValue
            }
          )
        ),
        cellClass: MultilineSubtitleCell.self,
        uuid: Preferences.General.openLinkInQuickViewMode.key
      )
      tabs.rows.append(quickViewRow)
    }

    var keyboardRow = Row(
      text: Strings.TabsSettings.autoOpenKeyboardTitle,
      detailText: Strings.TabsSettings.autoOpenKeyboardDescription,
      image: UIImage(braveSystemNamed: "leo.keyboard"),
      accessory: .view(
        SwitchAccessoryView(
          initialValue: Preferences.General.openKeyboardOnNTPSelection.value,
          valueChange: { [unowned self] newValue in
            Preferences.General.openKeyboardOnNTPSelection.value = newValue
          }
        )
      ),
      cellClass: MultilineSubtitleCell.self,
      uuid: Preferences.General.openKeyboardOnNTPSelection.key
    )
    tabs.rows.append(keyboardRow)

    return tabs
  }()


  private lazy var displaySection: Static.Section = {
    var display = Static.Section(
      header: .title(Strings.displaySettingsSection),
      rows: [],
      uuid: displaySectionUUID.uuidString
    )

    display.rows.append(
      .init(
        text: Strings.Settings.mediaRootSetting,
        selection: { [unowned self] in
          let vc = UIHostingController(rootView: MediaSettingsView(prefs: braveCore.profile.prefs))
          self.navigationController?.pushViewController(vc, animated: true)
        },
        image: UIImage(braveSystemNamed: "leo.media.player"),
        accessory: .disclosureIndicator,
        cellClass: MultilineValue1Cell.self
      )
    )

    let themeSubtitle = DefaultTheme(rawValue: Preferences.General.themeNormalMode.value)?
      .displayString
    var row = Row(
      text: Strings.themesDisplayBrightness,
      detailText: themeSubtitle,
      image: UIImage(braveSystemNamed: "leo.appearance"),
      accessory: .disclosureIndicator,
      cellClass: MultilineSubtitleCell.self
    )
    row.selection = { [unowned self] in
      let optionsViewController = OptionSelectionViewController<DefaultTheme>(
        options: DefaultTheme.normalThemesOptions,
        selectedOption: DefaultTheme(rawValue: Preferences.General.themeNormalMode.value),
        optionChanged: { [unowned self] _, option in
          Preferences.General.themeNormalMode.value = option.rawValue
          self.dataSource.reloadCell(row: row, section: display, displayText: option.displayString)
        }
      )
      optionsViewController.headerText = Strings.themesDisplayBrightness
      optionsViewController.navigationItem.title = Strings.themesDisplayBrightness

      let nightModeSection = Section(
        header: .title(Strings.NightMode.sectionTitle),
        rows: [
          .boolRow(
            title: Strings.NightMode.settingsTitle,
            detailText: Strings.NightMode.settingsDescription,
            option: Preferences.General.nightModeEnabled,
            image: UIImage(braveSystemNamed: "leo.theme.dark")
          )
        ],
        footer: .title(Strings.NightMode.sectionDescription)
      )

      optionsViewController.dataSource.sections.append(nightModeSection)
      self.navigationController?.pushViewController(optionsViewController, animated: true)
    }
    display.rows.append(row)
    // Growser-284: no "Change App Icon" row - every alternate is a Brave lion.
    display.rows.append(
      Row(
        text: Strings.NTP.settingsTitle,
        selection: { [unowned self] in
          self.navigationController?.pushViewController(
            NTPTableViewController(
              // Growser-290: no rewards.
              linkTapped: { [unowned self] request in
                self.tabManager.addTabAndSelect(
                  request,
                  isPrivate: false
                )
                self.dismiss(animated: true)
              }
            ),
            animated: true
          )
        },
        image: UIImage(braveSystemNamed: "leo.window.tab-new"),
        accessory: .disclosureIndicator,
        cellClass: MultilineValue1Cell.self
      )
    )

    // We do NOT persistently save page-zoom settings in Private Browsing
    if !tabManager.privateBrowsingManager.isPrivateBrowsing {
      display.rows.append(
        Row(
          text: Strings.PageZoom.settingsTitle,
          selection: { [weak self] in
            let controller = PageZoomSettingsController()
            controller.navigationItem.title = Strings.PageZoom.settingsTitle
            self?.navigationController?.pushViewController(controller, animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.font.size"),
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        )
      )
    }

    display.rows.append(
      Row(
        text: Strings.ShortcutButton.shortcutButtonTitle,
        selection: { [weak self] in
          guard let self else { return }
          let controller = UIHostingController(
            rootView: ShortcutButtonPickerView(
              prefs: braveCore.profile.prefs,
              isWalletAvailable: false  // Growser-287
            )
          )
          controller.navigationItem.title = Strings.ShortcutButton.shortcutButtonTitle
          navigationController?.pushViewController(controller, animated: true)
        },
        image: UIImage(braveSystemNamed: "leo.launch"),
        accessory: .disclosureIndicator,
        cellClass: MultilineValue1Cell.self
      )
    )

    // Growser-290: no "Hide Brave Rewards icon" row.

    return display
  }()

  // Growser-280: no vpnSettingsRow.

  // Growser-279: no leoSettingsRow - Leo is out of the product.

  private lazy var autofillSection: Static.Section = {
    return Section(
      header: .title(Strings.Autofill.settingsSectionTitle),
      rows: [
        Row(
          text: Strings.Autofill.managePasswordsTitle,
          selection: { [unowned self] in
            if let autofillDataManager = braveCore.defaultWebViewConfiguration.autofillDataManager {
              let viewModel = ManagePasswordsViewModel(autofillDataManager: autofillDataManager)
              let controller = UIHostingController(
                rootView:
                  ManagePasswordsView(viewModel: viewModel)
                  .environment(
                    \.openURL,
                    OpenURLAction { [weak self] url in
                      self?.settingsDelegate?.settingsOpenURLInNewTab(url)
                      return .handled
                    }
                  )
                  // Failed privacy-lock auth must exit the entire navigation flow: `dismiss` only pops the
                  // top SwiftUI screen, so from second order stack i.e detail/group
                  // we would not return to settings. `popToViewController` unwinds every `NavigationLink`-pushed host;
                  .environment(
                    \.autofillPrivacyLockExitOnFailure,
                    AutofillPrivacyLockExitOnFailureAction { [weak self] in
                      guard let self, let nav = self.navigationController else { return }
                      nav.popToViewController(self, animated: true)
                    }
                  )
              )

              navigationController?.pushViewController(controller, animated: true)
            } else {
              let loginsPasswordsViewController = LoginListViewController(
                passwordAPI: passwordAPI,
                windowProtection: windowProtection
              )
              loginsPasswordsViewController.settingsDelegate = self.settingsDelegate
              self.navigationController?.pushViewController(
                loginsPasswordsViewController,
                animated: true
              )
            }
          },
          image: UIImage(braveSystemNamed: "leo.key"),
          accessory: .disclosureIndicator
        )
      ]
    )
  }()

  private lazy var supportSection: Static.Section = {
    return Static.Section(
      header: .title(Strings.support),
      rows: [
        Row(
          text: Strings.reportABug,
          selection: { [unowned self] in
            self.settingsDelegate?.settingsOpenURLInNewTab(.brave.community)
            self.dismiss(animated: true)
          },
          image: UIImage(braveSystemNamed: "leo.bug"),
          cellClass: MultilineValue1Cell.self
        ),
        // Growser-299: no "Rate Growser" until the app has its own App Store
        // listing - the row wrote a review of Brave's app (id1052879175).
      ]
    )
  }()

  private lazy var aboutSection: Static.Section = {
    let version = String(
      format: Strings.versionTemplate,
      Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "",
      Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? ""
    )
    let titleLabel = UITableViewHeaderFooterView().then {
      $0.textLabel?.text = Strings.about
      if #unavailable(iOS 26.0) {
        $0.textLabel?.text = $0.textLabel?.text?.uppercased()
      }
      $0.isUserInteractionEnabled = true
      $0.addGestureRecognizer(
        UITapGestureRecognizer(target: self, action: #selector(tappedAboutHeader))
      )
    }
    let coreVersion =
      "BraveCore \(BraveCoreVersionInfo.braveCoreVersion) (\(BraveCoreVersionInfo.chromiumVersion))"
    return Static.Section(
      header: .autoLayoutView(titleLabel),
      rows: [
        Row(
          text: version,
          selection: { [unowned self, weak tabManager] in
            let device = UIDevice.current
            let actionSheet = UIAlertController(
              title: version,
              message: coreVersion,
              preferredStyle: .actionSheet
            )
            actionSheet.popoverPresentationController?.sourceView = self.view
            actionSheet.popoverPresentationController?.sourceRect = self.view.bounds
            let iOSVersion = "\(device.systemName) \(UIDevice.current.systemVersion)"

            let deviceModel = String(format: Strings.deviceTemplate, device.modelName, iOSVersion)
            let copyDebugInfoAction = UIAlertAction(
              title: Strings.copyAppInfoToClipboard,
              style: .default
            ) { _ in
              UIPasteboard.general.strings = [version, coreVersion, deviceModel]
            }

            let copyTabDebugInfoAction = UIAlertAction(
              title: Strings.copyTabsDebugToClipboard,
              style: .default
            ) { _ in
              guard let tabManager else { return }
              UIPasteboard.general.setSecureString(
                AppDebugComposer.composeTabDebug(tabManager),
                expirationDate: Date().addingTimeInterval(2.minutes)
              )
            }

            let copyAppInfoAction = UIAlertAction(
              title: Strings.copyAppSizeInfoToClipboard,
              style: .default
            ) { _ in
              Task { @MainActor in
                let size = await AppDebugComposer.composeAppSize()
                UIPasteboard.general.setSecureString(
                  size,
                  expirationDate: Date().addingTimeInterval(2.minutes)
                )
              }
            }

            actionSheet.addAction(copyDebugInfoAction)
            actionSheet.addAction(copyAppInfoAction)
            actionSheet.addAction(copyTabDebugInfoAction)
            actionSheet.addAction(
              UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel, handler: nil)
            )
            self.navigationController?.present(actionSheet, animated: true, completion: nil)
          },
          cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: Strings.privacyPolicy,
          selection: { [unowned self] in
            settingsDelegate?.settingsOpenURLInNewTab(.brave.privacy)
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        // Growser-299: no "Terms of use" - Growser has no terms of its own,
        // and the row opened Brave's.
        Row(
          text: Strings.settingsLicenses,
          selection: { [unowned self] in
            settingsDelegate?.settingsOpenURLInNewTab(.webUI.credits)  // Growser-321
          },
          accessory: .disclosureIndicator
        ),
      ]
    )
  }()

  private let debugSectionUUID = UUID().uuidString
  private lazy var debugSection: Static.Section = {
    var section = Static.Section(
      header: "Developer Options",
      rows: [
        Row(text: "Region: \(Locale.current.region?.identifier ?? "--")"),
        Row(
          text: "Sandbox Inspector",
          selection: { [unowned self] in
            let vc = UIHostingController(rootView: SandboxInspectorView())
            self.navigationController?.pushViewController(vc, animated: true)
          },
          accessory: .disclosureIndicator
        ),
        Row(
          text: "AdBlock Debugger",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(
              UIHostingController(rootView: AdBlockDebugView()),
              animated: true
            )
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: "User Agent Override",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(
              UIHostingController(rootView: UserAgentOverrideView()),
              animated: true
            )
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: "Secure Content State Debug",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(
              DebugLogViewController(type: .secureState),
              animated: true
            )
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        Row(text: "URP Code: \(UserReferralProgram.getReferralCode() ?? "--")"),
        Row(
          text: "BraveCore Switches",
          selection: { [unowned self] in
            let controller = UIHostingController(rootView: BraveCoreDebugSwitchesView())
            self.navigationController?.pushViewController(controller, animated: true)
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineSubtitleCell.self
        ),
        // Growser-281: no "View Brave News Debug Menu" row.
        Row(
          text: "View Brave Search Debug Menu",
          selection: { [unowned self] in
            self.displayBraveSearchDebugMenu()
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: "Consolidate Privacy Report Data",
          detailText:
            "This will force all data to consolidate. All stats for 'last 7 days' should be cleared and 'all time data' views should be preserved.",
          selection: {
            Preferences.PrivacyReports.nextConsolidationDate.value = Date().advanced(
              by: -2.days
            )
            PrivacyReportsManager.consolidateData(dayRange: -10)
          },
          cellClass: MultilineButtonCell.self
        ),
        // Growser-280: no "VPN Logs" row.
        // Growser-278: no "Brave Talk Logs" row.
        // Growser-279: no "Leo Logs" row.
        // Growser-282: no "Playlist Debug" row.
        // Growser-283: no "StoreKit Receipt Viewer" row.
        Row(
          text: "Onboarding Debug Menu",
          selection: { [unowned self] in
            self.navigationController?.pushViewController(
              RetentionPreferencesDebugMenuViewController(
                p3aUtilities: p3aUtilities,
                attributionManager: attributionManager,
                localState: localState
              ),
              animated: true
            )
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        ),
        Row(
          text: "Load all QA Links",
          selection: { [unowned self] in
            struct Links: Decodable {
              var links: [String]
            }
            Task.detached {
              do {
                let url = URL(
                  string:
                    "https://raw.githubusercontent.com/brave/qa-resources/master/testlinks.json"
                )!
                let data = try Data(contentsOf: url)
                let links = try JSONDecoder().decode(Links.self, from: data)
                let urls = links.links.compactMap(URL.init)
                await MainActor.run {
                  self.settingsDelegate?.settingsOpenURLs(urls, loadImmediately: false)
                  self.dismiss(animated: true)
                }
              } catch {}
            }
          },
          cellClass: MultilineButtonCell.self
        ),
        Row(
          text: "Create 1000 Tabs",
          selection: { [unowned self] in
            self.settingsDelegate?.settingsCreateFakeTabs()
            self.dismiss(animated: true)
          },
          cellClass: ButtonCell.self
        ),
        Row(
          text: "Create 1000 Bookmark Entries",
          selection: { [unowned self] in
            self.settingsDelegate?.settingsCreateFakeBookmarks()
            self.dismiss(animated: true)
          },
          cellClass: ButtonCell.self
        ),
        Row(
          text: "Create 10000 History Entries for past 10 days",
          selection: { [unowned self] in
            self.settingsDelegate?.settingsCreateFakeHistory()
            self.dismiss(animated: true)
          },
          cellClass: ButtonCell.self
        ),
        Row(
          text: "CRASH!!!",
          selection: { [unowned self] in
            let alert = UIAlertController(
              title: "Force crash?",
              message: nil,
              preferredStyle: .alert
            )
            alert.addAction(
              UIAlertAction(title: "Crash app", style: .destructive) { _ in
                fatalError()
              }
            )
            alert.addAction(UIAlertAction(title: "Cancel", style: .cancel, handler: nil))
            self.present(alert, animated: true, completion: nil)
          },
          cellClass: MultilineButtonCell.self
        ),
      ],
      uuid: debugSectionUUID
    )
    if !FeatureList.kUseProfileWebViewConfiguration.enabled {
      section.rows.append(
        Row(
          text: "Injected Scripts",
          selection: { [unowned self] in
            let controller = UIHostingController(rootView: UserScriptsDebugView())
            self.navigationController?.pushViewController(controller, animated: true)
          },
          accessory: .disclosureIndicator,
          cellClass: MultilineValue1Cell.self
        )
      )
    }
    if AppConstants.isOfficialBuild {
      section.rows.append(
        Row(
          text: "Hide Developer Options",
          selection: { [unowned self] in
            Preferences.Debug.developerOptionsEnabled.value = false
            self.dataSource.sections.removeAll(where: { $0.uuid == self.debugSectionUUID })
          },
          image: nil,
          cellClass: ButtonCell.self
        )
      )
    }
    return section
  }()

  private var aboutHeaderTapCount: Int = 0

  @objc private func tappedAboutHeader() {
    if kBraveDeveloperOptionsCode.isEmpty || !AppConstants.isOfficialBuild
      || Preferences.Debug.developerOptionsEnabled.value
    {
      // No code supplied, or already showing the developer options
      return
    }

    aboutHeaderTapCount += 1
    if aboutHeaderTapCount == 5 {
      aboutHeaderTapCount = 0

      // We don't need to hide the screen or apply window protection in any way, so just going
      // to use LAContext directly. Fine to ignore entirely if the user doesn't have a passcode
      // enabled.
      let context = LAContext()
      if context.canEvaluatePolicy(.deviceOwnerAuthentication, error: nil) {
        context.evaluatePolicy(
          .deviceOwnerAuthentication,
          localizedReason: "Grant access to developer options"
        ) { [weak self] success, _ in
          if success {
            DispatchQueue.main.async {
              self?.displayDeveloperOptions()
            }
          }
        }
      }
    }
  }

  private func displayDeveloperOptions() {
    let alert = UIAlertController(
      title: "Show Developer Options",
      message: nil,
      preferredStyle: .alert
    )
    alert.addTextField { textField in
      textField.isSecureTextEntry = true
    }
    alert.addAction(.init(title: "Cancel", style: .cancel))
    alert.addAction(
      .init(
        title: "Submit",
        style: .default,
        handler: { [unowned alert, unowned self] _ in
          guard let textField = alert.textFields?.first else { return }
          if textField.text == kBraveDeveloperOptionsCode {
            Preferences.Debug.developerOptionsEnabled.value = true
            self.dataSource.sections.append(self.debugSection)
          }
        }
      )
    )
    present(alert, animated: true)
  }

  private func setUpSections() {
    // Growser-287: no Web3 settings row - the wallet is out.
    self.dataSource.sections = self.makeSections()
  }

  // Growser-280: no presentVPNPaywall(), enableVPNTapped() or
  // dismissVPNHeaderTapped().
}

private final class BraveAccountIconCell: UITableViewCell, Cell {
  func configure(row: Row) {
    var content = defaultContentConfiguration()
    let scaledValue = UIFontMetrics.default.scaledValue(for: 26)
    content.image = UIImage(sharedNamed: "brave.logo")?.preparingThumbnail(
      of: .init(width: scaledValue, height: scaledValue)
    )
    content.text = L10nUtils.string(messageId: .SETTINGS_BRAVE_ACCOUNT_ROW_TITLE)
    contentConfiguration = content
    accessoryType = .disclosureIndicator
  }
}

private class AppIconCell: UITableViewCell, Cell {
  func configure(row: Row) {
    var content = defaultContentConfiguration()
    content.image = row.image
    content.text = row.text
    content.imageProperties.cornerRadius = 6
    let scaledValue = UIFontMetrics.default.scaledValue(for: 24)
    content.imageProperties.maximumSize = .init(width: scaledValue, height: scaledValue)
    content.imageProperties.strokeColor = UIColor(white: 0, alpha: 0.1)
    content.imageProperties.strokeWidth = 1
    contentConfiguration = content
    accessoryType = .disclosureIndicator
  }
}
