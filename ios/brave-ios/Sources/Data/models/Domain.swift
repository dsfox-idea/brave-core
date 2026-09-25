// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import CoreData
import Foundation
import Preferences
import Shared
import UIKit
import os.log

public final class Domain: NSManagedObject, CRUD {

  @NSManaged public var url: String?
  @NSManaged public var visits: Int32
  // not currently used. Should be used once proper frecency code is in.
  @NSManaged public var topsite: Bool
  @NSManaged public var blockedFromTopSites: Bool  // don't show ever on top sites

  // swift-format-ignore
  @NSManaged public var shield_allOff: NSNumber?
  // swift-format-ignore
  @NSManaged public var shield_adblockAndTp: NSNumber?

  // swift-format-ignore
  @available(*, deprecated, message: "Per domain HTTPSE shield is currently unused.")
  @NSManaged public var shield_httpse: NSNumber?

  // swift-format-ignore
  @NSManaged public var shield_noScript: NSNumber?
  // swift-format-ignore
  @NSManaged public var shield_fpProtection: NSNumber?
  // swift-format-ignore
  @NSManaged public var shield_safeBrowsing: NSNumber?

  @NSManaged public var bookmarks: NSSet?

  // swift-format-ignore
  @NSManaged public var wallet_permittedAccounts: String?
  // swift-format-ignore
  @NSManaged public var zoom_level: NSNumber?
  // swift-format-ignore
  @NSManaged public var wallet_solanaPermittedAcccounts: String?
  // swift-format-ignore
  @NSManaged public var wallet_cardanoPermittedAccounts: String?

  /// A string version of the shield shred level
  // swift-format-ignore
  @NSManaged public var shield_shredLevel: String?

  /// A string version of the shield ad-block and tracking protection
  // swift-format-ignore
  @NSManaged public var shield_blockAdsAndTrackingLevel: String?

  private var urlComponents: URLComponents? {
    return URLComponents(string: url ?? "")
  }

  private static let containsEthereumPermissionsPredicate = NSPredicate(
    format: "wallet_permittedAccounts != nil && wallet_permittedAccounts != ''"
  )
  private static let containsSolanaPermissionsPredicate = NSPredicate(
    format: "wallet_solanaPermittedAcccounts != nil && wallet_solanaPermittedAcccounts != ''"
  )
  private static let containsCardanoPermissionsPredicate = NSPredicate(
    format: "wallet_cardanoPermittedAccounts != nil && wallet_cardanoPermittedAccounts != ''"
  )

  /// A domain can be created in many places,
  /// different save strategies are used depending on its relationship(eg. attached to a Bookmark) or browsing mode.
  enum SaveStrategy {
    /// Immediately saves to the persistent store.
    case persistentStore
    /// Targets persistent store but the database save will occur at other place in code, for example after a whole Bookmark is saved.
    case delayedPersistentStore
    /// Saves to an in-memory store. Usually only used when in private browsing mode.
    case inMemory

    fileprivate var saveContext: NSManagedObjectContext {
      switch self {
      case .persistentStore, .delayedPersistentStore:
        return DataController.newBackgroundContext()
      case .inMemory:
        return DataController.newBackgroundContextInMemory()
      }
    }
  }

  // MARK: - Public interface

  public class func frc() -> NSFetchedResultsController<Domain> {
    let context = DataController.viewContext
    let fetchRequest = NSFetchRequest<Domain>()
    fetchRequest.entity = Domain.entity(context)
    fetchRequest.sortDescriptors = [NSSortDescriptor(key: "url", ascending: false)]

    return NSFetchedResultsController(
      fetchRequest: fetchRequest,
      managedObjectContext: context,
      sectionNameKeyPath: nil,
      cacheName: nil
    )
  }

  public class func getOrCreate(forUrl url: URL, persistent: Bool) -> Domain {
    let context = persistent ? DataController.viewContext : DataController.viewContextInMemory
    let saveStrategy: SaveStrategy = persistent ? .persistentStore : .inMemory

    return getOrCreateInternal(url, context: context, saveStrategy: saveStrategy)
  }

  /// Returns saved Domain for url or nil if it doesn't exist.
  /// Always called on main thread context.
  public class func getPersistedDomain(for url: URL) -> Domain? {
    Domain.first(where: NSPredicate(format: "url == %@", url.domainURL.absoluteString))
  }

  // MARK: Shields

  public class func allDomainsWithExplicitShieldLevel() -> [Domain]? {
    Domain.all(
      where: NSPredicate(format: "shield_blockAdsAndTrackingLevel != nil")
    )
  }

  /// Returns all domains with a given `SiteShredLevel`s rawValue, or `nil`
  /// for Domains with no shred level assigned.
  class func allDomainsWithAutoShredLevel(
    _ rawShredLevel: String?,
    context: NSManagedObjectContext
  ) -> [Domain]? {
    let shredLevelPredicate: NSPredicate
    if let rawShredLevel {
      shredLevelPredicate = NSPredicate(
        format: "shield_shredLevel == %@",
        rawShredLevel
      )
    } else {
      shredLevelPredicate = NSPredicate(format: "shield_shredLevel == nil")
    }
    return Domain.all(where: shredLevelPredicate, context: context)
  }

  /// Get all URLs from Domain objects with the given Shred Level.
  /// NOTE: This method is performed on background context, so only the URL's
  /// are returned for thread-safety.
  public class func allURLsWithShredLevel(
    rawShredLevel: String,
    isGlobalShredLevel: Bool
  ) async -> [URL] {
    let context = DataController.newBackgroundContext()
    return await context.perform {
      let mapToURL: (Domain) -> URL? = { domain -> URL? in
        guard let urlString = domain.url,
          let url = URL(string: urlString),
          !url.isNewTabURL,
          !InternalURL.isValid(url: url)
        else {
          return nil
        }
        return url
      }
      let domainsWithExplicitRequestedShredLevel =
        Domain.allDomainsWithAutoShredLevel(rawShredLevel, context: context) ?? []
      guard isGlobalShredLevel else {
        return domainsWithExplicitRequestedShredLevel.compactMap(mapToURL)
      }
      // Default value is requested shred level, so fetch Domain's using default value
      let domainsUsingDefaultShredLevel =
        Domain.allDomainsWithAutoShredLevel(nil, context: context) ?? []

      // A Domain may be created with non-secure scheme, then a separate Domain
      // may be created with the secure scheme after https upgrade.
      // WKWebsiteDataStore stores data without a scheme, so we should remove
      // duplicate Domain's from the domainsWithDefaultShredLevel if the user explicitly
      // assigned a shred level on it's baseDomain.
      guard
        let domainsWithCustomShredLevel = Domain.all(
          where: NSPredicate(format: "shield_shredLevel != nil"),
          context: context
        ),
        !domainsWithCustomShredLevel.isEmpty
      else {
        // no domains with custom assigned shred level assigned,
        // so we can return all without shred level assigned
        return (domainsWithExplicitRequestedShredLevel + domainsUsingDefaultShredLevel).compactMap(
          mapToURL
        )
      }
      let baseDomainsToExclude: Set<String> = Set(
        domainsWithCustomShredLevel.compactMap({
          guard let urlString = $0.url, let url = URL(string: urlString) else {
            // if no url, not usable for shred
            return nil
          }
          return url.baseDomain
        })
      )
      let domainsUsingDefaultShredLevelFiltered =
        domainsUsingDefaultShredLevel
        .filter { domain in
          guard let urlString = domain.url, let url = URL(string: urlString),
            let baseDomain = url.baseDomain
          else {
            // if no url, not usable for shred
            return false
          }
          // exclude domain if on exclusion list
          return !baseDomainsToExclude.contains(baseDomain)
        }

      return (domainsWithExplicitRequestedShredLevel + domainsUsingDefaultShredLevelFiltered)
        .compactMap(mapToURL)
    }
  }

  public static func clearInMemoryDomains() {
    Domain.deleteAll(predicate: nil, context: .new(inMemory: true))
  }

  @MainActor public class func allDomainsWithMigratableShieldLevel() -> [Domain]? {
    return Domain.all(
      where: NSPredicate(
        format: "shield_adblockAndTp != nil AND shield_blockAdsAndTrackingLevel == nil"
      )
    )
  }

  public class func totalDomainsWithFingerprintingProtectionLoweredFromGlobal() -> Int {
    guard let domains = Domain.all(where: NSPredicate(format: "shield_fpProtection != nil"))
    else {
      return 0  // Can't be lower than off
    }
    return domains.filter({ $0.shield_fpProtection?.boolValue == false }).count
  }

  public class func totalDomainsWithFingerprintingProtectionIncreasedFromGlobal() -> Int {
    guard let domains = Domain.all(where: NSPredicate(format: "shield_fpProtection != nil"))
    else {
      return 0  // Can't be higher than on
    }
    return domains.filter({ $0.shield_fpProtection?.boolValue == true }).count
  }

  // Growser-287: no wallet permissions - the wallet is out of the product.
}

// MARK: - Internal implementations

extension Domain {
  // Currently required, because not `syncable`
  public static func entity(_ context: NSManagedObjectContext) -> NSEntityDescription {
    return NSEntityDescription.entity(forEntityName: "Domain", in: context)!
  }

  /// Returns a Domain for given URL or creates a new object if it doesn't exist.
  /// Note: Save operation can block main thread.
  class func getOrCreateInternal(
    _ url: URL,
    context: NSManagedObjectContext = DataController.viewContext,
    saveStrategy: SaveStrategy
  ) -> Domain {
    let domainString = url.domainURL.absoluteString
    if let domain = Domain.first(
      where: NSPredicate(format: "url == %@", domainString),
      context: context
    ) {
      return domain
    }

    var newDomain: Domain!

    // Domains are usually accesed on view context, but when the Domain doesn't exist,
    // we have to switch to a background context to avoid writing on view context(bad practice).
    let writeContext =
      context.concurrencyType == .mainQueueConcurrencyType ? saveStrategy.saveContext : context

    writeContext.performAndWait {
      newDomain = Domain(entity: Domain.entity(writeContext), insertInto: writeContext)
      newDomain.url = domainString

      let shouldSave = saveStrategy == .persistentStore || saveStrategy == .inMemory

      if shouldSave && writeContext.hasChanges {
        do {
          try writeContext.save()
        } catch {
          Logger.module.error("Domain save error: \(error.localizedDescription)")
        }
      }
    }

    guard let domainOnCorrectContext = context.object(with: newDomain.objectID) as? Domain else {
      assertionFailure("Could not retrieve domain on correct context")
      return newDomain
    }

    return domainOnCorrectContext
  }

  public class func deleteAll(_ completionOnMain: @escaping () -> Void) {
    Domain.deleteAll(completion: completionOnMain)
  }

  class func getForUrl(_ url: URL) -> Domain? {
    let domainString = url.domainURL.absoluteString
    return Domain.first(where: NSPredicate(format: "url == %@", domainString))
  }

  // MARK: Shields

  /// Returns `url` but switches the scheme from `http` <-> `https`
  private func domainForInverseHttpScheme(context: NSManagedObjectContext) -> Domain? {

    guard var urlComponents = self.urlComponents else { return nil }

    // Flip the scheme if valid

    switch urlComponents.scheme {
    case "http": urlComponents.scheme = "https"
    case "https": urlComponents.scheme = "http"
    default: return nil
    }

    guard let url = urlComponents.url else { return nil }

    // Return the flipped scheme version of `url`.
    // Not saving here, save happens in at higher level in `perform` method.
    return Domain.getOrCreateInternal(url, context: context, saveStrategy: .delayedPersistentStore)
  }

  // Growser-287: no wallet permissions.

  @MainActor public class func allDomainsWithExlicitShieldSettings() -> [Domain] {
    let predicate = NSCompoundPredicate(orPredicateWithSubpredicates: [
      NSPredicate(format: "shield_allOff != nil"),
      NSPredicate(format: "shield_blockAdsAndTrackingLevel != nil"),
      NSPredicate(format: "shield_noScript != nil"),
      NSPredicate(format: "shield_fpProtection != nil"),
      NSPredicate(format: "shield_shredLevel != nil"),
    ])
    return Domain.all(where: predicate) ?? []
  }
}
