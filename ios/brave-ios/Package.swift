// swift-tools-version: 6.2.1
// The swift-tools-version declares the minimum version of Swift required to build this package.

import Foundation
import PackageDescription

// News, Playlist (+JS), Onboarding, Browser (Favicons, Bookmarks, History, Passwords, Reader Mode, Settings, Sync),
// VPN, Rewards, Shields (Privacy, De-Amp, Downloaders, Content Blockers, ...), NTP, Networking,

var package = Package(
  name: "Brave",
  defaultLocalization: "en",
  platforms: [.iOS(.v18)],
  products: [
    .library(name: "Brave", targets: ["Brave"]),
    .library(name: "Shared", targets: ["Shared"]),
    .library(
      name: "BraveCore",
      targets: ["BraveCore", "PartitionAllocSupport"]
    ),
    .library(name: "BraveShared", targets: ["BraveShared"]),
    .library(name: "BraveShields", targets: ["BraveShields"]),
    .library(name: "BraveUI", targets: ["BraveUI"]),
    .library(name: "DesignSystem", targets: ["DesignSystem", "NalaAssets"]),
    // Growser-287: no BraveWallet library.
    .library(name: "Data", targets: ["Data"]),
    .library(name: "DataImporter", targets: ["DataImporter"]),
    .library(name: "BrowserIntentsModels", targets: ["BrowserIntentsModels"]),
    .library(name: "BraveWidgetsModels", targets: ["BraveWidgetsModels"]),
    .library(name: "Strings", targets: ["Strings"]),
    .library(name: "BraveStrings", targets: ["BraveStrings"]),
    // Growser-280: no BraveVPN library.
    // Growser-281: no BraveNews library.
    // Growser-279: no AIChat library.
    // Growser-283: no BraveStore library.
    .library(name: "Favicon", targets: ["Favicon"]),
    .library(name: "FaviconModels", targets: ["FaviconModels"]),
    .library(name: "SpeechRecognition", targets: ["SpeechRecognition"]),
    .library(name: "Onboarding", targets: ["Onboarding"]),
    .library(name: "Growth", targets: ["Growth"]),
    .library(name: "RuntimeWarnings", targets: ["RuntimeWarnings"]),
    .library(name: "CodableHelpers", targets: ["CodableHelpers"]),
    .library(name: "GRDWireGuardKit", targets: ["GRDWireGuardKit"]),
    .library(name: "Preferences", targets: ["Preferences"]),
    .library(name: "PrivateCDN", targets: ["PrivateCDN"]),
    .library(name: "CertificateUtilities", targets: ["CertificateUtilities"]),
    // Growser-282: no Playlist library.
    .library(name: "UserAgent", targets: ["UserAgent"]),
    .library(name: "CredentialProviderUI", targets: ["CredentialProviderUI"]),
    // Growser-282: no PlaylistUI library.
    .library(name: "BrowserMenu", targets: ["BrowserMenu"]),
    .library(name: "Web", targets: ["Web"]),
    // Growser-278: no BraveTalk library.
    // Growser-283: no Origin library.
    .plugin(name: "IntentBuilderPlugin", targets: ["IntentBuilderPlugin"]),
    .plugin(name: "LoggerPlugin", targets: ["LoggerPlugin"]),
  ],
  dependencies: [
    .package(url: "https://github.com/SnapKit/SnapKit", from: "5.0.1"),
    .package(url: "https://github.com/cezheng/Fuzi", from: "3.1.3"),
    .package(url: "https://github.com/airbnb/lottie-spm", from: "4.4.3"),
    .package(url: "https://github.com/SDWebImage/SDWebImage", exact: "5.10.3"),
    // Growser-287: no SDWebImageSwiftUI or Swift-BigInt - only BraveWallet
    // used them.
    // Growser-281: no FeedKit - only BraveNews parsed RSS with it.
    .package(url: "https://github.com/apple/swift-collections", from: "1.0.0"),
    .package(url: "https://github.com/siteline/SwiftUI-Introspect", from: "26.0.2"),
    .package(url: "https://github.com/apple/swift-algorithms", from: "1.0.0"),
    .package(url: "https://github.com/devxoul/Then", from: "2.7.0"),
    // Growser-280: no GuardianConnect - Guardian's VPN SDK went with the VPN.
    .package(url: "https://github.com/pointfreeco/swift-custom-dump", from: "0.6.0"),
    .package(
      url: "https://github.com/venmo/Static",
      revision: "622a6804d39515600ead16e6259cb5d5e50f40df"
    ),
    // Growser-278: JitsiMeet went with BraveTalk, and with it the
    // jitsi/webrtc package its own manifest pulls - a SECOND WebRTC, 302 MB,
    // cloned from GitHub at the 35-50 KB/s this network gets (growser#264).
    // That clone was the single slowest thing in bringing iOS up.
  ],
  targets: [
    .target(
      name: "Brave",
      dependencies: [
        "BraveShared",
        "Shared",
        // Growser-287: no BraveWallet.
        "BraveCore",
        "PartitionAllocSupport",
        "BraveUI",
        "DesignSystem",
        "Data",
        "DataImporter",
        "Fuzi",
        "SnapKit",
        "Static",
        "SDWebImage",
        "Then",
        "BrowserIntentsModels",
        "BraveWidgetsModels",
        // Growser-280: no BraveVPN.
        // Growser-281: no BraveNews.
        // Growser-279: no AIChat.
        // Growser-283: no BraveStore.
        "Onboarding",
        "Growth",
        "SpeechRecognition",
        "CodableHelpers",
        "Preferences",
        "Favicon",
        "CertificateUtilities",
        // Growser-282: no Playlist.
        "UserAgent",
        .product(name: "Lottie", package: "lottie-spm"),
        .product(name: "Collections", package: "swift-collections"),
        // Growser-287: two debug views used Algorithms without importing it,
        // through BraveWallet's dependency; now it is declared where it is used.
        .product(name: "Algorithms", package: "swift-algorithms"),
        // Growser-282: no PlaylistUI.
        "BrowserMenu",
        "Web",
        "BraveShields",
        // Growser-278: no BraveTalk.
        // Growser-283: no Origin.
      ],
      exclude: [
        "Frontend/UserContent/UserScripts/AllFrames",
        "Frontend/UserContent/UserScripts/MainFrame",
        "Frontend/UserContent/UserScripts/Sandboxed",
        // Growser-278: Brave Talk is out of the product. The files stay on
        // disk so upstream merges do not conflict on them; excluding them is
        // what keeps them out of the app.
        "Frontend/Settings/Debug/BraveTalkLogsView.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/BraveTalkScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveTalkScript.js",
        // Growser-279: Leo is out of the product, on the same terms.
        "Frontend/Browser/BrowserViewController/BVC+AIChat.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Sandboxed/BraveLeoScriptHandler.swift",
        // Growser-280: the VPN is out of the product, on the same terms.
        "Frontend/Browser/BrowserViewController/BVC+VPN.swift",
        "Frontend/Settings/Debug/VPNLogsViewController.swift",
        // Growser-281: Brave News is out of the product, on the same terms.
        "Frontend/Browser/NewTabPage/NewTabPageFeedOverlayView.swift",
        "Frontend/Browser/NewTabPage/Sections/BraveNewsSectionProvider.swift",
        // Growser-282: Playlist is out of the product, on the same terms.
        "Frontend/Browser/Playlist",
        "Frontend/Browser/BrowserViewController/BVC+Playlist.swift",
        "Frontend/Browser/Helpers/PlaylistTabHelper.swift",
        "Frontend/Settings/Features/PlaylistSettingsViewController.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/PlaylistScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistScript.js",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/PlaylistSwizzlerScript.js",
        // Growser-283: Brave's paid-product layer is out, on the same terms.
        "BraveSkus",
        "Frontend/Browser/BrowserViewController/BVC+Origin.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/BraveSkusScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSkusScript.js",
        // Growser-287: the wallet is out of the product, on the same terms:
        // its UI glue, the Ethereum, Solana and Cardano providers pages talk to,
        // and ENS/SNS/Unstoppable name resolution with its interstitial.
        "Wallet",
        "Frontend/BraveNotifications/BraveWallet",
        "Frontend/Settings/Debug/BraveWallet",
        "Frontend/Browser/BrowserViewController/BVC+Wallet.swift",
        "Frontend/Browser/BrowserViewController/BVC+Web3NameService.swift",
        "Frontend/Browser/Handlers/Web3DomainHandler.swift",
        "Frontend/Browser/Helpers/WalletTabHelper.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Internal/Web3NameServiceScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/CardanoProviderScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/EthereumProviderScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/SolanaProviderScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletCardanoProviderScript.js",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletEthereumProviderScript.js",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/WalletSolanaProviderScript.js",
        "Assets/InterstitialPages/Pages/Web3Domain.html",
        "Assets/InterstitialPages/Styles/Web3Domain.css",
        // Growser-290: Brave Rewards and Brave Ads are out of the product, on
        // the same terms: the Rewards service and panel, ads notifications,
        // search-result ads, DeviceCheck enrolment and the internals pages.
        "DeviceCheck",
        "Extensions/Rewards",
        "Frontend/Rewards",
        "Frontend/BraveRewards",
        "Frontend/BraveNotifications/BraveRewards",
        "Frontend/Settings/Debug/RewardsInternals",
        "Frontend/Settings/Features/BraveRewardsSettingsViewController.swift",
        "Frontend/Browser/BrowserViewController/BVC+Rewards.swift",
        "Frontend/Browser/NewTabPage/Notifications",
        "Frontend/Browser/Search/BraveSearchResultAdManager.swift",
        "Frontend/Browser/SearchResultAdClickedInfoBar.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/ScriptHandlers/Paged/BraveSearchResultAdScriptHandler.swift",
        "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSearchResultAdScript.js",
        // Growser-284: every alternate app icon is a Brave lion, so the
        // picker is out and the app wears the G only.
        "Frontend/Settings/Display/AltIconsModel.swift",
        "Frontend/Settings/Display/AltIconsView.swift",
      ],
      resources: [
        .copy("Assets/About/AboutHome.html"),
        .copy("Assets/__firefox__.js"),
        .copy("Assets/AllFramesAtDocumentEnd.js"),
        .copy("Assets/AllFramesAtDocumentEndSandboxed.js"),
        .copy("Assets/AllFramesAtDocumentStart.js"),
        .copy("Assets/AllFramesAtDocumentStartSandboxed.js"),
        .copy("Assets/MainFrameAtDocumentEnd.js"),
        .copy("Assets/MainFrameAtDocumentEndSandboxed.js"),
        .copy("Assets/MainFrameAtDocumentStart.js"),
        .copy("Assets/MainFrameAtDocumentStartSandboxed.js"),
        .copy("Assets/Fonts/FiraSans-Bold.ttf"),
        .copy("Assets/Fonts/FiraSans-BoldItalic.ttf"),
        .copy("Assets/Fonts/FiraSans-Book.ttf"),
        .copy("Assets/Fonts/FiraSans-Italic.ttf"),
        .copy("Assets/Fonts/FiraSans-Light.ttf"),
        .copy("Assets/Fonts/FiraSans-Medium.ttf"),
        .copy("Assets/Fonts/FiraSans-Regular.ttf"),
        .copy("Assets/Fonts/FiraSans-SemiBold.ttf"),
        .copy("Assets/Fonts/FiraSans-UltraLight.ttf"),
        .copy("Assets/Fonts/NewYorkMedium-Bold.otf"),
        .copy("Assets/Fonts/NewYorkMedium-BoldItalic.otf"),
        .copy("Assets/Fonts/NewYorkMedium-Regular.otf"),
        .copy("Assets/Fonts/NewYorkMedium-RegularItalic.otf"),
        .copy("Assets/InterstitialPages/Pages/BlockedDomain.html"),
        .copy("Assets/InterstitialPages/Pages/HTTPBlocked.html"),
        .copy("Assets/InterstitialPages/Images/Info.svg"),
        .copy("Assets/InterstitialPages/Images/warning-triangle-outline.svg"),
        .copy("Assets/InterstitialPages/Styles/BlockedDomain.css"),
        .copy("Assets/InterstitialPages/Styles/InterstitialStyles.css"),
        .copy("Assets/Lottie/shred.json"),
        .copy("Assets/SearchPlugins"),
        .copy("Frontend/Reader/Reader.css"),
        .copy("Frontend/Reader/Reader.html"),
        .copy("Frontend/Reader/ReaderViewLoading.html"),
        .copy("Frontend/Browser/NewTabPage/Backgrounds/Assets/NTP_Images/corwin-prescott-3.jpg"),
        .copy("Frontend/Browser/Favorites/Data/top_sites_by_region.json"),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/BraveSearchScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/DomainSpecific/Paged/FrameCheckWrapper.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/CookieControlScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/FarblingProtectionScript.js"
        ),
        .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/gpc.js"),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/MediaBackgroundingScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/RequestBlockingScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/TrackingProtectionStats.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Paged/YoutubeQualityScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/TextContentDistillerScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/DarkReaderScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/BraveTranslateScript.js"
        ),
        .copy("Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/DeAmpScript.js"),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/FaviconScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/ResourceDownloaderScript.js"
        ),
        .copy(
          "Frontend/UserContent/UserScripts/Scripts_Dynamic/Scripts/Sandboxed/SiteStateListenerScript.js"
        ),
        .copy("WebFilters/ContentBlocker/Lists/block-ads.json"),
        .copy("WebFilters/ContentBlocker/Lists/block-cookies.json"),
        .copy("WebFilters/ContentBlocker/Lists/block-trackers.json"),
        .copy("WebFilters/ContentBlocker/Lists/mixed-content-upgrade.json"),
        .copy("WebFilters/ShieldStats/Adblock/Resources/ABPFilterParserData.dat"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "Shared",
      dependencies: [
        "BraveCore",
        "Strings",
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveShared",
      dependencies: ["BraveCore", "Shared", "Preferences"],
      // Growser-290: ads are out of the product, and BraveAds with them.
      exclude: ["Extensions/BraveAdsExtensions.swift"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "CertificateUtilities",
      dependencies: ["Shared"],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(
      name: "CertificateUtilitiesTests",
      dependencies: [
        "CertificateUtilities", "BraveShared", "BraveCore",
        "PartitionAllocSupport",
      ],
      exclude: ["Certificates/self-signed.conf"],
      resources: [
        .copy("Certificates/certviewer/brave.com.cer"),
        .copy("Certificates/certviewer/github.com.cer"),
      ]
    ),
    .target(name: "BraveStrings", dependencies: ["Strings", "Preferences"]),
    .target(
      name: "Growth",
      dependencies: [
        // Growser-280: no BraveVPN - and BraveUI (currentScene), which used to
        // arrive through it, is now asked for by name.
        "BraveUI", "Shared", "BraveShared", "Strings", "SnapKit", "CertificateUtilities",
        .product(name: "OrderedCollections", package: "swift-collections"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "SpeechRecognition",
      dependencies: ["BraveUI", "Shared", "BraveShared", "Preferences", "Data"],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "BraveUI",
      dependencies: [
        "Strings",
        "DesignSystem",
        "SDWebImage",
        "SnapKit",
        .product(name: "SwiftUIIntrospect", package: "SwiftUI-Introspect"),
        "Then",
        "Static",
        "Preferences",
        "Shared",
        .product(name: "Lottie", package: "lottie-spm"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(name: "BraveUITests", dependencies: ["BraveUI"]),
    .target(
      name: "BraveShields",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveUI",
        "Data",
        "DesignSystem",
        "Favicon",
        "Preferences",
        "Shared",
        "Strings",
        "Web",
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(
      name: "BraveShieldsTests",
      dependencies: ["BraveShields", "Data", "Preferences", "TestHelpers", "Web"]
    ),
    .target(
      name: "DesignSystem",
      dependencies: ["Then", "NalaAssets"]
    ),
    .binaryTarget(name: "NalaAssets", path: "../../../out/ios_current_link/NalaAssets.xcframework"),
    .binaryTarget(
      name: "PartitionAllocSupport",
      path: "../../../out/ios_current_link/PartitionAllocSupport.xcframework"
    ),
    .binaryTarget(name: "BraveCore", path: "../../../out/ios_current_link/BraveCore.xcframework"),
    .binaryTarget(
      name: "GRDWireGuardKit",
      path: "../third_party/GRDWireGuardKit/GRDWireGuardKit.xcframework"
    ),
    .target(
      name: "Data",
      dependencies: ["BraveShared", "Strings", "Preferences", "Shared"],
      // Growser-287: the wallet's own entities. The model keeps them, so a
      // store written by an older build still opens; nothing reads them.
      exclude: [
        "models/WalletUserAsset.swift", "models/WalletUserAssetBalance.swift",
        "models/WalletUserAssetGroup.swift",
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(
      name: "DataImporter",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveStrings",
        "BraveUI",
        "DesignSystem",
        "Growth",
        "Strings",
        .product(name: "Collections", package: "swift-collections"),
        .product(name: "SwiftUIIntrospect", package: "SwiftUI-Introspect"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    // Growser-287: the BraveWallet target and its tests are not declared, so
    // their sources are not built.
    .target(
      name: "BrowserIntentsModels",
      dependencies: ["Shared"],
      sources: ["BrowserIntents.intentdefinition", "CustomIntentHandler.swift"],
      plugins: ["IntentBuilderPlugin"]
    ),
    .target(
      name: "BraveWidgetsModels",
      dependencies: ["FaviconModels"],
      sources: [
        "BraveWidgets.intentdefinition", "LockScreenFavoriteIntentHandler.swift",
        "FavoritesWidgetData.swift", "DisabledShortcutsWidgetData.swift",
      ],
      plugins: ["IntentBuilderPlugin", "LoggerPlugin"]
    ),
    .target(name: "TestHelpers", dependencies: ["Data", "BraveShared"]),
    // Growser-280: the BraveVPN target and its tests are not declared, so
    // Sources/BraveVPN and Tests/BraveVPNTests are not built.
    // Growser-281: the BraveNews target and its tests are not declared, so
    // Sources/BraveNews and Tests/BraveNewsTests are not built.
    // Growser-279: the AIChat target is not declared, so Sources/AIChat is
    // not built.
    // Growser-283: the BraveStore and Origin targets are not declared, so their
    // sources are not built.
    .target(name: "Preferences", dependencies: ["Shared"], plugins: ["LoggerPlugin"]),
    .target(
      name: "Onboarding",
      dependencies: [
        "BraveCore",
        "BraveShared",
        "BraveStrings",
        "BraveUI",
        "DesignSystem",
        "Growth",
        .product(name: "Lottie", package: "lottie-spm"),
        "Preferences",
        "Shared",
        "SnapKit",
      ],
      // Growser-280: the VPN promotions are out of the product with the VPN.
      // Growser-290: so is the Rewards agreement, with Rewards.
      // Growser-286: and its animation, which shipped with nothing to play it.
      exclude: [
        "VPNNotifications", "Callouts/OnboardingRewardsAgreementViewController.swift",
        "LottieAssets/onboarding-rewards.json",
      ],
      resources: [
        .copy("LottieAssets/playlist-confetti.json"),
        .copy("WelcomeFocus/Resources/LottieAssets"),
        .copy("WelcomeFocus/Resources/Videos"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(name: "CodableHelpers"),
    .target(name: "FaviconModels", dependencies: ["Shared"]),
    .target(
      name: "Favicon",
      dependencies: [
        "FaviconModels",
        "BraveCore",
        "BraveShared",
        "Shared",
        "SDWebImage",
      ],
      resources: [
        .copy("Assets/top_sites.json"),
        .copy("Assets/TopSites"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .target(name: "UserAgent", dependencies: ["Preferences", "BraveCore"]),
    .target(
      name: "CredentialProviderUI",
      dependencies: ["BraveCore", "DesignSystem", "BraveShared", "Strings", "BraveUI"]
    ),
    .testTarget(name: "UserAgentTests", dependencies: ["UserAgent", "Brave"]),
    .testTarget(name: "SharedTests", dependencies: ["Shared"]),
    .testTarget(
      name: "BraveSharedTests",
      dependencies: ["BraveShared", "Preferences"]
    ),
    .testTarget(
      name: "DataTests",
      dependencies: ["Data", "TestHelpers", "BraveShields"],
      // Growser-287: the wallet entities they test are not built.
      exclude: [
        "WalletUserAssetBalanceTests.swift", "WalletUserAssetGroupTests.swift",
        "WalletUserAssetTests.swift",
      ]
    ),
    .testTarget(
      name: "ClientTests",
      dependencies: ["Brave", "BraveStrings", "TestHelpers", "Web"],
      // Growser-283: the SKUS glue it tests is not built. Growser-287: nor is
      // the Solana provider.
      exclude: ["Helpers/BraveSkusWebHelperTests.swift", "SolanaProviderScriptHandlerTests.swift"],
      resources: [
        .copy("Resources/debouncing.json"),
        .copy("Resources/content-blocking.json"),
        .copy("Resources/filter-lists.json"),
        .copy("Resources/google-search-plugin.xml"),
        .copy("Resources/duckduckgo-search-plugin.xml"),
        .copy("Resources/ad-block-resources/resources.json"),
        .copy("Resources/filter-rules/iodkpdagapdfkphljnddpjlldadblomo.txt"),
        .copy("Resources/html/index.html"),
        .copy("Resources/scripts/farbling-tests.js"),
        .copy("Resources/scripts/request-blocking-tests.js"),
        .copy("Resources/scripts/cosmetic-filter-tests.js"),
      ]
    ),
    .target(name: "Strings"),
    .target(name: "RuntimeWarnings"),
    .target(name: "PrivateCDN", dependencies: ["SDWebImage"]),
    // Growser-282: the Playlist and PlaylistUI targets and their tests are not
    // declared, so their sources are not built.
    .testTarget(name: "PrivateCDNTests", dependencies: ["PrivateCDN"]),
    .testTarget(
      name: "GrowthTests",
      dependencies: ["Growth", "Shared", "BraveShared"]  // Growser-280: no BraveVPN
    ),
    .target(
      name: "BrowserMenu",
      dependencies: [
        // Growser-280: no BraveVPN or GuardianConnect.
        "DesignSystem", "BraveUI", "Preferences", "Strings", "BraveStrings",
        "BraveShields",  // Growser-287: no BraveWallet
      ]
    ),
    .target(
      name: "Web",
      dependencies: [
        "BraveCore", "FaviconModels", "BraveShared", "Shared", "CertificateUtilities",
        "BraveStrings", "Strings",
        .product(name: "OrderedCollections", package: "swift-collections"),
      ],
      plugins: ["LoggerPlugin"]
    ),
    .testTarget(name: "BrowserMenuTests", dependencies: ["BrowserMenu"]),
    .plugin(name: "IntentBuilderPlugin", capability: .buildTool()),
    .plugin(name: "LoggerPlugin", capability: .buildTool()),
    // Growser-278: the BraveTalk target and its tests are not declared, so
    // Sources/BraveTalk and Tests/BraveTalkTests are not built.
  ],
  swiftLanguageModes: [.v5],
  cxxLanguageStandard: .cxx17
)

let iosRootDirectory = URL(string: PackageDescription.Context.packageDirectory)!.absoluteString
let isStripAbsolutePathsFromDebugSymbolsEnabled = {
  do {
    let env = try String(contentsOfFile: "\(iosRootDirectory)/../../.env", encoding: .utf8)
      .split(separator: "\n")
      .map { $0.split(separator: "=").map { $0.trimmingCharacters(in: .whitespacesAndNewlines) } }
    return env.contains(where: { $0.first == "use_remoteexec" && $0.last == "true" })
  } catch {
    return false
  }
}()

for target in package.targets where target.type == .regular || target.type == .test {
  var settings = target.swiftSettings ?? []
  if isStripAbsolutePathsFromDebugSymbolsEnabled {
    settings.append(
      .unsafeFlags(
        [
          "-debug-prefix-map", "\(iosRootDirectory)=../../brave/ios/brave-ios",
        ],
        .when(configuration: .debug)
      )
    )
  }
  // Approchable Concurrency feature flags
  settings.append(contentsOf: [
    .enableUpcomingFeature("DisableOutwardActorInference"),
    .enableUpcomingFeature("GlobalActorIsolatedTypesUsability"),
    .enableUpcomingFeature("InferIsolatedConformances"),
    .enableUpcomingFeature("InferSendableFromCaptures"),
    .enableUpcomingFeature("NonisolatedNonsendingByDefault"),
  ])
  target.swiftSettings = settings
}
