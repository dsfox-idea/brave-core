/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import android.app.Activity;
import android.util.Base64;

import androidx.annotation.IntDef;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave.browser.brave_origin.BraveOriginServiceFactory;
import org.chromium.brave.browser.util.BraveDomainsUtils;
import org.chromium.brave.browser.util.ServicesEnvironment;
import org.chromium.brave_origin.mojom.BraveOriginSettingsHandler;
import org.chromium.brave_origin.mojom.OriginActivationLimit;
import org.chromium.build.annotations.Contract;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.PurchaseModel;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.policy.BravePolicyConstants;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveOriginPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

/** Utility class for managing Brave Origin subscription preferences. */
@NullMarked
public class BraveOriginSubscriptionPrefs {
    private static final String TAG = "BraveOriginSubsPrefs";
    private static final String ORIGIN_SKU_HOSTNAME_PART = "origin";

    // JSON field names for credential summary response
    private static final String JSON_FIELD_ACTIVE = "active";
    private static final String JSON_FIELD_REMAINING_CREDENTIAL_COUNT =
            "remaining_credential_count";

    // JSON field names for order creation request
    private static final String JSON_FIELD_TYPE = "type";
    private static final String JSON_FIELD_RAW_RECEIPT = "raw_receipt";
    private static final String JSON_FIELD_PACKAGE = "package";
    private static final String JSON_FIELD_SUBSCRIPTION_ID = "subscription_id";
    private static final String JSON_VALUE_ANDROID = "android";

    /** Outcome of a post-purchase credential fetch. */
    @IntDef({
        CredentialFetchResult.SUCCESS,
        CredentialFetchResult.FAILED,
        CredentialFetchResult.ACTIVATION_LIMIT_EXTENDABLE,
        CredentialFetchResult.ACTIVATION_LIMIT_REACHED
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface CredentialFetchResult {
        int SUCCESS = 0;
        int FAILED = 1;

        /**
         * The subscription has been activated on as many devices as it allows, and the payment
         * service says that budget can be raised. The user has to ask for it, via {@link
         * #extendActivationLimit}.
         */
        int ACTIVATION_LIMIT_EXTENDABLE = 2;

        /**
         * At the activation limit with no extension available - typically because one was already
         * granted. Distinct from {@link #FAILED} because the subscription is fine and the user
         * needs telling, rather than a generic failure.
         */
        int ACTIVATION_LIMIT_REACHED = 3;
    }

    /**
     * Callback that fires when fetchOrderCredentials completes. Only one observer at a time (the
     * currently-open BraveOriginPreferences).
     */
    @Nullable private static Callback<Integer> sCredentialsFetchedCallback;

    /**
     * Order awaiting an activation-limit extension, and the request body that identifies it. Both
     * are set when a credential fetch failed because the order is out of activations, and are held
     * here rather than handed to the UI because they are only ever passed straight back to {@link
     * #extendActivationLimit}. The failure path never persists the order ID to prefs, so this is
     * the only copy.
     */
    @Nullable private static String sPendingExtendOrderId;

    @Nullable private static String sPendingExtendReceiptPayload;

    /**
     * True while a {@link #createFetchOrder} chain is running. The fetch lives entirely in memory,
     * so this guards {@link #resumeCredentialFetchIfNeeded} from starting a second, concurrent
     * fetch when both the startup hook and the settings screen detect the same interrupted state.
     * Reset in {@link #notifyCredentialsFetched}, which every terminal path of the fetch routes
     * through.
     */
    private static boolean sFetchInProgress;

    /**
     * True while {@code profile} can still be used to reach native prefs and services.
     *
     * <p>Every Origin flow here spans a long async chain (Play Store billing queries, Skus mojo
     * round-trips), so a profile captured when the chain started can be destroyed before a callback
     * runs. A destroyed profile keeps its Java object alive, so a plain null check still passes
     * while {@code UserPrefs.get()} returns null.
     */
    @Contract("null -> false")
    public static boolean isProfileUsable(@Nullable Profile profile) {
        return profile != null && !profile.shutdownStarted();
    }

    /**
     * Returns the {@link PrefService} for {@code profile}, or null when prefs are out of reach.
     *
     * <p>{@link UserPrefs#get} is declared non-null but returns null for a destroyed
     * BrowserContext, so its result is checked as well as the profile.
     */
    private static @Nullable PrefService getPrefs(@Nullable Profile profile) {
        if (!isProfileUsable(profile)) {
            return null;
        }
        return UserPrefs.get(profile);
    }

    /**
     * Registers a one-shot callback that will be invoked on the UI thread when
     * fetchOrderCredentials finishes. Any previously registered callback is replaced.
     *
     * @param callback Called with a {@link CredentialFetchResult}.
     * @param profile Used to re-check fetch state and guard against a race where the fetch
     *     completed between the caller's isFetchingCredentials() check and this call.
     */
    public static void setCredentialsFetchedCallback(
            @Nullable Callback<Integer> callback, @Nullable Profile profile) {
        sCredentialsFetchedCallback = callback;
        // If the fetch completed between the caller's isFetchingCredentials() check and this
        // call, notifyCredentialsFetched() ran with no listener and the result was lost.
        // The only way isFetchingCredentials() flips to false is a successful fetch (failure
        // paths leave the order ID empty), so synthesize success.
        if (callback != null && !isFetchingCredentials(profile)) {
            notifyCredentialsFetched(CredentialFetchResult.SUCCESS);
        }
    }

    /**
     * Returns true when a purchase has been made (subscription active) but fetchOrderCredentials
     * has not yet completed (order ID is still empty).
     */
    public static boolean isFetchingCredentials(@Nullable Profile profile) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            return false;
        }
        return prefService.getBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID)
                && prefService.getString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID).isEmpty()
                && !prefService.getString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID).isEmpty();
    }

    /**
     * Resumes an interrupted post-purchase credential fetch left over from a prior session.
     *
     * <p>The fetch ({@link #createFetchOrder}) runs entirely in memory: if the browser is killed
     * before {@link #fetchOrderCredentials} writes the order ID, the prefs stay in the "fetching"
     * state ({@link #isFetchingCredentials} stays true) but no fetch is running. Nothing else
     * re-triggers it on the next launch - the Play Store {@link #verifyPurchase} restore is skipped
     * while the subscription pref is already active - so the Origin settings screen would otherwise
     * show the "Disabling features" spinner forever. Call this on startup to restart the fetch from
     * the persisted purchase token. Unlike {@link #verifyPurchase} it issues no Play Store billing
     * query and never clears local state, so a transient billing outage cannot un-enroll the user.
     *
     * <p>A failed fetch leaves the same state behind as a killed one - the order ID is only
     * persisted on success - so this is also what re-runs the fetch when the user returns to the
     * Origin settings screen after a failure, rather than leaving them on a spinner that nothing
     * will ever resolve.
     *
     * @param profile The profile to use for the operation.
     */
    public static void resumeCredentialFetchIfNeeded(@Nullable Profile profile) {
        resumeCredentialFetchIfNeeded(profile, /* openSettings= */ true);
    }

    /**
     * @param openSettings Whether to open the Origin settings screen to show the fetch progress.
     *     Callers that are already on that screen pass false.
     */
    public static void resumeCredentialFetchIfNeeded(
            @Nullable Profile profile, boolean openSettings) {
        if (sFetchInProgress || !isFetchingCredentials(profile)) {
            return;
        }
        // isFetchingCredentials() already guarantees the prefs are reachable and that the purchase
        // token is non-empty.
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            return;
        }
        String purchaseToken = prefService.getString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID);
        createFetchOrder(profile, purchaseToken);
        if (openSettings) {
            // Mirror verifyPurchase(): open the Origin settings screen so the user sees the spinner
            // while credentials finish fetching and is then prompted to restart, since the enforced
            // policies only take effect after a restart.
            BraveOriginSettingsLauncherHelper.showOriginSettingsForRestart();
        }
    }

    /**
     * Fires the one-shot credentials-fetched callback, if registered.
     *
     * @param result How the credential fetch ended.
     */
    private static void notifyCredentialsFetched(@CredentialFetchResult int result) {
        sFetchInProgress = false;
        Callback<Integer> callback = sCredentialsFetchedCallback;
        sCredentialsFetchedCallback = null;
        if (callback != null) {
            PostTask.postTask(TaskTraits.UI_DEFAULT, callback.bind(result));
        }
    }

    /**
     * Queries Google Play for an existing Origin purchase and restores it if found. This handles
     * the case where a user changes devices - the purchase exists in their Google account but the
     * local prefs are empty.
     *
     * @param profile The profile to use for preference storage
     */
    public static void verifyPurchase(@Nullable Profile profile) {
        MutableLiveData<PurchaseModel> _activePurchases = new MutableLiveData<>();
        LiveData<PurchaseModel> activePurchases = _activePurchases;
        InAppPurchaseWrapper inAppPurchaseWrapper = InAppPurchaseWrapper.getInstance();
        // Suppress toasts during startup query so devices without Google Play
        // don't show "Billing service is not available" on every launch.
        inAppPurchaseWrapper.setSuppressToasts(true);
        inAppPurchaseWrapper.queryPurchases(
                _activePurchases, InAppPurchaseWrapper.SubscriptionProduct.ORIGIN);
        LiveDataUtil.observeOnce(
                activePurchases,
                activePurchaseModel -> {
                    boolean purchaseFound = activePurchaseModel != null;
                    setIsSubscriptionActive(profile, purchaseFound);
                    if (purchaseFound) {
                        setOriginPackageName(profile);
                        setOriginProductId(profile, activePurchaseModel.getProductId());
                        setOriginPurchaseToken(profile, activePurchaseModel.getPurchaseToken());
                        // We only reach verifyPurchase() while the subscription pref is inactive
                        // (see the BraveActivity guard), so a found purchase means a prior Play
                        // Store purchase is being auto-restored. setOriginPurchaseToken() above
                        // already started the credential fetch, so open the Origin settings screen:
                        // it shows the spinner while credentials are fetched and prompts a restart
                        // once the policies are enforced.
                        BraveOriginSettingsLauncherHelper.showOriginSettingsForRestart();
                    } else {
                        setOriginProductId(profile, "");
                        setOriginPurchaseToken(profile, "");
                    }
                });
    }

    /**
     * Sets the Origin subscription active status for the given profile.
     *
     * @param profile The profile to use for preference storage
     * @param value The subscription active status
     */
    public static void setIsSubscriptionActive(@Nullable Profile profile, boolean value) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "setIsSubscriptionActive prefs are unavailable");
            return;
        }
        prefService.setBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID, value);
    }

    /**
     * Gets the Play Store Origin subscription active status for the given profile.
     *
     * <p>Note: this only reflects Play Store purchases recorded in {@code
     * BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID} by the in-app billing callback. Users whose Origin
     * subscription was purchased on desktop and linked via their Brave account will read as
     * inactive here. For an any-source sync check suitable for UI gating, call {@link
     * #getIsCredentialSummaryActiveCached}. For the authoritative fresh value, call {@link
     * #requestCredentialSummary} (async, via the Skus mojo service).
     *
     * @param profile The profile to use for preference retrieval
     * @return The Play Store subscription active status, or false if the profile is unusable
     */
    public static boolean getIsSubscriptionActive(@Nullable Profile profile) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "getIsSubscriptionActive prefs are unavailable");
            return false;
        }
        return prefService.getBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID);
    }

    /**
     * Sets the Origin subscription purchase token for the given profile.
     *
     * @param profile The profile to use for preference storage
     * @param token The purchase token
     */
    public static void setOriginPurchaseToken(@Nullable Profile profile, String token) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "setOriginPurchaseToken prefs are unavailable");
            return;
        }
        if (prefService.getString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID).equals(token)
                && !prefService.getString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID).isEmpty()) {
            return;
        }
        // It means we don't have a Play Store subscription anymore or
        // we have a new one.
        resetSubscriptionLinkedStatus(profile);
        prefService.setString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID, token);
        if (!token.isEmpty()) {
            createFetchOrder(profile, token);
        }
    }

    /**
     * Sets the Origin subscription package name for the given profile.
     *
     * @param profile The profile to use for preference storage
     */
    public static void setOriginPackageName(@Nullable Profile profile) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "setOriginPackageName prefs are unavailable");
            return;
        }
        prefService.setString(
                BravePref.BRAVE_ORIGIN_PACKAGE_NAME_ANDROID,
                ContextUtils.getApplicationContext().getPackageName());
    }

    /**
     * Sets the Origin subscription product ID for the given profile.
     *
     * @param profile The profile to use for preference storage
     * @param productId The product ID
     */
    public static void setOriginProductId(@Nullable Profile profile, String productId) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "setOriginProductId prefs are unavailable");
            return;
        }
        prefService.setString(BravePref.BRAVE_ORIGIN_PRODUCT_ID_ANDROID, productId);
    }

    /**
     * Checks if the Origin subscription is linked for the given profile.
     *
     * @param profile The profile to use for preference retrieval
     * @return True if subscription is linked, false otherwise
     */
    public static boolean isSubscriptionLinked(@Nullable Profile profile) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "isSubscriptionLinked prefs are unavailable");
            return false;
        }

        return prefService.getInteger(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_LINK_STATUS_ANDROID) != 0;
    }

    /**
     * Resets the subscription linked status for the given profile.
     *
     * @param profile The profile to use for preference storage
     */
    private static void resetSubscriptionLinkedStatus(@Nullable Profile profile) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "resetSubscriptionLinkedStatus prefs are unavailable");
            return;
        }
        prefService.setInteger(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_LINK_STATUS_ANDROID, 0);
    }

    private static void createFetchOrder(@Nullable Profile profile, String purchaseToken) {
        // Growser-126: no payment service to create an order with.
        notifyCredentialsFetched(CredentialFetchResult.FAILED);
    }


    public static void extendActivationLimit(@Nullable Profile profile) {
        // Growser-126: no payment service to raise an activation budget with.
        sPendingExtendOrderId = null;
        sPendingExtendReceiptPayload = null;
        notifyCredentialsFetched(CredentialFetchResult.FAILED);
    }


    public static void requestCredentialSummary(
            @Nullable Profile profile, @Nullable Callback<Boolean> callback) {
        // Growser-126: no payment service to ask, so no subscription.
        setIsCredentialSummaryActiveCached(false);
        if (callback != null) {
            callback.onResult(false);
        }
    }

    /**
     * Caches the authoritative any-source Origin active status returned by the most recent {@link
     * #requestCredentialSummary} callback. Unlike the Play-Store-only {@link
     * #getIsSubscriptionActive}, the Skus credential summary covers both Play Store purchases and
     * desktop-linked subscriptions, so this cache is the correct source for sync "is Origin active"
     * gating on Android.
     */
    private static void setIsCredentialSummaryActiveCached(boolean value) {
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_ORIGIN_CREDENTIAL_SUMMARY_CACHED, value);
    }

    /**
     * Synchronous "is Origin effectively active" check for UI-thread gating of promo surfaces.
     * Returns the value written by the most recent {@link #requestCredentialSummary} callback, or
     * false if no refresh has run yet. Callers that need the authoritative fresh value should call
     * {@link #requestCredentialSummary} directly; this getter is intended for hot paths that need a
     * cheap sync answer and tolerate state as of the last refresh.
     */
    public static boolean getIsCredentialSummaryActiveCached() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BravePreferenceKeys.BRAVE_ORIGIN_CREDENTIAL_SUMMARY_CACHED, false);
    }


    /**
     * Clears all Origin subscription preferences for the given profile.
     *
     * @param profile The profile to use for preference clearing
     */
    public static void clearOriginSubscriptionPrefs(@Nullable Profile profile) {
        PrefService prefService = getPrefs(profile);
        if (prefService == null) {
            Log.e(TAG, "clearOriginSubscriptionPrefs prefs are unavailable");
            return;
        }

        prefService.setBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID, false);
        prefService.setString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_PRODUCT_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_PACKAGE_NAME_ANDROID, "");
        resetSubscriptionLinkedStatus(profile);
    }

    /**
     * Opens the Brave Origin preferences settings screen.
     *
     * @param activity The activity to use for launching the settings
     */
    public static void openOriginPreferences(Activity activity) {
        if (activity.isFinishing()) {
            Log.e(TAG, "openOriginPreferences activity is finishing");
            return;
        }
        SettingsNavigationFactory.createSettingsNavigation()
                .startSettings(activity, BraveOriginPreferences.class);
    }

    /**
     * Checks if a policy value should be inverted when mapping to/from UI state. DISABLED policies
     * need inversion (true = disabled = unchecked in UI). ENABLED policies don't need inversion
     * (true = enabled = checked in UI).
     *
     * @param policyKey The policy key to check
     * @return true if the policy value should be inverted, false otherwise
     */
    public static boolean isPolicyInverted(@Nullable String policyKey) {
        if (policyKey == null) {
            return false;
        }
        // Growser-261: the four *Disabled policies this switch named were
        // dropped in growser#62; every policy left is an *Enabled one.
        return false;
    }

    /**
     * Convenience method to check a single policy. Delegates to {@link #checkPoliciesAsync}.
     *
     * @param profile The profile to use for the operation
     * @param policyKey The policy key to check (e.g., BravePolicyConstants.BRAVE_REWARDS_DISABLED)
     * @param callback Called with the policy value (true if disabled, false if not disabled)
     */
    public static void checkPolicyAsync(
            @Nullable Profile profile, String policyKey, @Nullable Callback<Boolean> callback) {
        if (callback == null) {
            return;
        }
        checkPoliciesAsync(profile, Map.of(policyKey, callback));
    }

    /**
     * Checks multiple policies asynchronously with a single subscription check. This is more
     * efficient than calling checkPolicyAsync multiple times, as it only checks subscription status
     * once.
     *
     * @param profile The profile to use for the operation
     * @param policyCallbacks Map of policy keys to their callbacks. Each callback will be called
     *     with true if the feature is disabled by policy, false otherwise.
     */
    public static void checkPoliciesAsync(
            @Nullable Profile profile, Map<String, Callback<Boolean>> policyCallbacks) {
        if (policyCallbacks == null || policyCallbacks.isEmpty()) {
            return;
        }

        // If Brave Origin feature is not enabled or the profile is gone, policies are not
        // applicable
        if (!ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_ORIGIN)
                || !isProfileUsable(profile)) {
            for (Callback<Boolean> callback : policyCallbacks.values()) {
                if (callback != null) {
                    callback.onResult(false);
                }
            }
            return;
        }

        // Check subscription status once for all policies
        requestCredentialSummary(
                profile,
                (isSubscriptionActive) -> {
                    // If subscription is not active, all features are enabled (not disabled). The
                    // profile is re-checked because the summary request is asynchronous.
                    if (!isSubscriptionActive || !isProfileUsable(profile)) {
                        for (Callback<Boolean> callback : policyCallbacks.values()) {
                            if (callback != null) {
                                callback.onResult(false);
                            }
                        }
                        return;
                    }

                    // Subscription is active, check each policy
                    BraveOriginServiceFactory factory = BraveOriginServiceFactory.getInstance();
                    BraveOriginSettingsHandler handler =
                            factory.getBraveOriginSettingsHandler(profile, null);
                    if (handler == null) {
                        for (Callback<Boolean> callback : policyCallbacks.values()) {
                            if (callback != null) {
                                callback.onResult(false);
                            }
                        }
                        return;
                    }

                    // Track remaining callbacks to know when to close handler
                    final AtomicInteger remaining = new AtomicInteger(policyCallbacks.size());

                    for (Map.Entry<String, Callback<Boolean>> entry : policyCallbacks.entrySet()) {
                        String policyKey = entry.getKey();
                        Callback<Boolean> callback = entry.getValue();

                        handler.getPolicyValue(
                                policyKey,
                                (value) -> {
                                    if (callback != null) {
                                        boolean isDisabled =
                                                isPolicyInverted(policyKey)
                                                        ? (value != null && value)
                                                        : (value == null || !value);
                                        callback.onResult(isDisabled);
                                    }

                                    // Close handler when all callbacks are done
                                    if (remaining.decrementAndGet() == 0) {
                                        handler.close();
                                    }
                                });
                    }
                });
    }
}
