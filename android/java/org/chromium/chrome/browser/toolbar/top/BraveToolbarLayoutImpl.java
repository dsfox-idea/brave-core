/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.top;

import static org.chromium.build.NullUtil.assertNonNull;
import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.PorterDuff;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.ContextCompat;
import androidx.core.widget.ImageViewCompat;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.BraveReflectionUtil;
import org.chromium.base.Log;
import org.chromium.base.MathUtils;
import org.chromium.base.ThreadUtils;
import org.chromium.base.supplier.MonotonicObservableSupplier;
import org.chromium.base.task.AsyncTask;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveConstants;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.brave_stats.BraveStatsUtil;
import org.chromium.chrome.browser.custom_layout.popup_window_tooltip.PopupWindowTooltip;
import org.chromium.chrome.browser.customtabs.features.toolbar.CustomTabToolbar;
import org.chromium.chrome.browser.local_database.BraveStatsTable;
import org.chromium.chrome.browser.local_database.DatabaseHelper;
import org.chromium.chrome.browser.local_database.SavedBandwidthTable;
import org.chromium.chrome.browser.media.PictureInPicture;
import org.chromium.chrome.browser.omnibox.BraveLocationBarCoordinator;
import org.chromium.chrome.browser.omnibox.LocationBarCoordinator;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettingsObserver;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.shields.BraveShieldsMenuObserver;
import org.chromium.chrome.browser.shields.BraveShieldsUtils;
import org.chromium.chrome.browser.shields.BraveUnifiedPanelHandler;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabHidingType;
import org.chromium.chrome.browser.tab.TabSelectionType;
import org.chromium.chrome.browser.tabmodel.IncognitoStateProvider;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabModelObserver;
import org.chromium.chrome.browser.tabmodel.TabModelSelectorTabObserver;
import org.chromium.chrome.browser.theme.ThemeColorProvider;
import org.chromium.chrome.browser.theme.ThemeUtils;
import org.chromium.chrome.browser.toolbar.ToolbarDataProvider;
import org.chromium.chrome.browser.toolbar.ToolbarProgressBar;
import org.chromium.chrome.browser.toolbar.ToolbarTabController;
import org.chromium.chrome.browser.toolbar.back_button.BackButtonCoordinator;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarConfiguration;
import org.chromium.chrome.browser.toolbar.bottom.BottomToolbarVariationManager;
import org.chromium.chrome.browser.toolbar.forward_button.ForwardButtonCoordinator;
import org.chromium.chrome.browser.toolbar.home_button.HomeButton;
import org.chromium.chrome.browser.toolbar.home_button.HomeButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.BraveMenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.menu_button.MenuButtonCoordinator;
import org.chromium.chrome.browser.toolbar.reload_button.ReloadButtonCoordinator;
import org.chromium.chrome.browser.toolbar.signin_button.SigninButtonCoordinator;
import org.chromium.chrome.browser.toolbar.top.NavigationPopup.HistoryDelegate;
import org.chromium.chrome.browser.user_education.UserEducationHelper;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.youtube_script_injector.BraveYouTubeScriptInjectorNativeHelper;
import org.chromium.components.feature_engagement.Tracker;
import org.chromium.content_public.browser.NavigationHandle;
import org.chromium.mojo.bindings.ConnectionErrorHandler;
import org.chromium.mojo.system.MojoException;
import org.chromium.ui.base.DeviceFormFactor;
import org.chromium.ui.base.ViewUtils;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.interpolators.Interpolators;
import org.chromium.ui.resources.dynamics.ViewResourceAdapter;
import org.chromium.ui.util.ColorUtils;
import org.chromium.ui.widget.Toast;
import org.chromium.url.GURL;

import java.net.URL;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.function.Supplier;

public abstract class BraveToolbarLayoutImpl extends ToolbarLayout
        implements BraveToolbarLayout,
                OnClickListener,
                View.OnLongClickListener,
                ConnectionErrorHandler { // Growser-271: no rewards observers
    private static final String TAG = "BraveToolbar";

    private static final int URL_FOCUS_TOOLBAR_BUTTONS_TRANSLATION_X_DP = 10;

    public static boolean mShouldShowPlaylistMenu;

    private final DatabaseHelper mDatabaseHelper = DatabaseHelper.getInstance();

    private ImageButton mBraveWalletButton;
    private ImageButton mBraveShieldsButton;
    private ImageButton mYouTubePipButton;
    private HomeButton mHomeButton;
    private FrameLayout mWalletLayout;
    private FrameLayout mShieldsLayout;
    private FrameLayout mYouTubePipLayout;
    private BraveUnifiedPanelHandler mUnifiedPanelHandler;

    // TabModelSelectorTabObserver setups observer at the ctor
    @SuppressWarnings("UnusedVariable")
    private TabModelSelectorTabObserver mTabModelSelectorTabObserver;

    // TabModelSelectorTabModelObserver setups observer at the ctor
    @SuppressWarnings("UnusedVariable")
    private TabModelSelectorTabModelObserver mTabModelSelectorTabModelObserver;

    private BraveShieldsContentSettings mBraveShieldsContentSettings;
    private BraveShieldsContentSettingsObserver mBraveShieldsContentSettingsObserver;
    private View mBraveWalletBadge;
    private ImageView mWalletIcon;
    private int mCurrentToolbarColor;
    // True once the Brave buttons sit inside the location bar text box, which happens on tablet
    // only. The location bar then paints their background, and the segment drawables the phone
    // toolbar relies on no longer apply.
    private boolean mBraveButtonsInLocationBar;

    @Nullable private final Runnable mToolbarSnapshotCaptureRunnable;

    private PopupWindowTooltip mShieldsPopupWindowTooltip;

    private boolean mIsBottomControlsVisible;

    private ColorStateList mDarkModeTint;
    private ColorStateList mLightModeTint;

    private final Set<Integer> mTabsWithWalletIcon =
            Collections.synchronizedSet(new HashSet<Integer>());

    public BraveToolbarLayoutImpl(Context context, AttributeSet attrs) {
        super(context, attrs);

        if (DeviceFormFactor.isNonMultiDisplayContextOnTablet(getContext())) {
            mToolbarSnapshotCaptureRunnable = this::requestToolbarSnapshotCapture;
        } else {
            mToolbarSnapshotCaptureRunnable = null;
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        removeCallbacks(mToolbarSnapshotCaptureRunnable);
        super.onDetachedFromWindow();
    }

    @Override
    public void destroy() {
        if (mUnifiedPanelHandler != null) {
            mUnifiedPanelHandler.destroy();
        }
        if (mBraveShieldsContentSettings != null) {
            mBraveShieldsContentSettings.removeObserver(mBraveShieldsContentSettingsObserver);
        }
        super.destroy();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mWalletLayout = findViewById(R.id.brave_wallet_button_layout);
        mShieldsLayout = findViewById(R.id.brave_shields_button_layout);
        mYouTubePipLayout = findViewById(R.id.brave_youtube_pip_layout);
        mBraveWalletButton = findViewById(R.id.brave_wallet_button);
        mBraveShieldsButton = findViewById(R.id.brave_shields_button);
        mYouTubePipButton = findViewById(R.id.brave_youtube_pip_button);
        mHomeButton = findViewById(R.id.home_button);
        mBraveWalletBadge = findViewById(R.id.wallet_notfication_badge);
        if (mWalletLayout != null) {
            mWalletIcon = mWalletLayout.findViewById(R.id.brave_wallet_button);
        }

        // Use the same tints as the omnibox status icon, so the icons Brave adds inside the URL
        // bar match the ones upstream puts there.
        mDarkModeTint = ThemeUtils.getThemedToolbarIconTint(getContext(), false);
        mLightModeTint = ThemeUtils.getThemedToolbarIconTint(getContext(), true);

        if (mHomeButton != null) {
            mHomeButton.setOnLongClickListener(this);
        }

        if (mBraveShieldsButton != null) {
            mBraveShieldsButton.setClickable(true);
            mBraveShieldsButton.setOnClickListener(this);
            mBraveShieldsButton.setOnLongClickListener(this);
            BraveTouchUtils.ensureMinTouchTarget(mBraveShieldsButton);
        }

        if (mBraveWalletButton != null) {
            mBraveWalletButton.setClickable(true);
            mBraveWalletButton.setOnClickListener(this);
            mBraveWalletButton.setOnLongClickListener(this);
            BraveTouchUtils.ensureMinTouchTarget(mBraveWalletButton);
        }

        if (mYouTubePipButton != null) {
            mYouTubePipButton.setClickable(true);
            mYouTubePipButton.setOnClickListener(this);
            mYouTubePipButton.setOnLongClickListener(this);
            BraveTouchUtils.ensureMinTouchTarget(mYouTubePipButton);
        }

        maybeAnchorBraveButtonsInLocationBar();

        mUnifiedPanelHandler = new BraveUnifiedPanelHandler(getContext());
        mUnifiedPanelHandler.addObserver(
                new BraveShieldsMenuObserver() {
                    @Override
                    public void onMenuTopShieldsChanged(boolean isOn, boolean isTopShield) {
                        Tab currentTab = getToolbarDataProvider().getTab();
                        if (currentTab == null) {
                            return;
                        }
                        if (isTopShield) {
                            updateBraveShieldsButtonState(currentTab);
                        }
                        if (currentTab.isLoading()) {
                            currentTab.stopLoading();
                        }
                        currentTab.reloadIgnoringCache();
                    }
                });
        mBraveShieldsContentSettingsObserver =
                new BraveShieldsContentSettingsObserver() {
                    @Override
                    public void blockEvent(int tabId, String blockType, String subresource) {
                        mUnifiedPanelHandler.addStat(tabId, blockType, subresource);
                        Tab currentTab = getToolbarDataProvider().getTab();
                        if (currentTab == null || currentTab.getId() != tabId) {
                            return;
                        }
                        if (!isIncognito()
                                && OnboardingPrefManager.getInstance().isBraveStatsEnabled()
                                && (blockType.equals(
                                                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_ADS)
                                        || blockType.equals(
                                                BraveShieldsContentSettings
                                                        .RESOURCE_IDENTIFIER_TRACKERS))) {
                            addStatsToDb(blockType, subresource, currentTab.getUrl().getSpec());
                        }
                    }

                    @Override
                    public void savedBandwidth(long savings) {
                        if (!isIncognito()
                                && OnboardingPrefManager.getInstance().isBraveStatsEnabled()) {
                            addSavedBandwidthToDb(savings);
                        }
                    }
                };
        // Initially show shields off image. Shields button state will be updated when tab is
        // shown and loading state is changed.
        updateBraveShieldsButtonState(null);
        if (BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)) {
            if (getMenuButtonCoordinator() != null
                    && isMenuButtonOnBottomControls()
                    && BottomToolbarConfiguration.isToolbarTopAnchored()) {
                getMenuButtonCoordinator().setVisibility(false);
            }
        }

        // Growser-271: no rewards button beside it, so Shields ends the row.
        updateShieldsLayoutBackground(true);
    }

    public String getLocationBarQuery() {
        if (getLocationBar() instanceof BraveLocationBarCoordinator) {
            String query =
                    ((BraveLocationBarCoordinator) getLocationBar())
                            .getUrlBarTextWithoutAutocomplete();
            return query;
        }
        return "";
    }

    public void clearOmniboxFocus() {
        if (getLocationBar() instanceof BraveLocationBarCoordinator) {
            ((BraveLocationBarCoordinator) getLocationBar()).clearOmniboxFocus();
        }
    }

    public boolean isUrlBarFocused() {
        if (getLocationBar() instanceof BraveLocationBarCoordinator) {
            BraveLocationBarCoordinator coordinator =
                    (BraveLocationBarCoordinator) getLocationBar();
            assertNonNull(coordinator.getLocationBarMediator());
            return coordinator.getLocationBarMediator().isUrlBarFocused();
        }
        return false;
    }

    @Override
    public void onConnectionError(MojoException e) {
        // Growser-273: the playlist service this reconnected to is gone.
    }

    @Override
    protected void onNativeLibraryReady() {
        super.onNativeLibraryReady();
        mBraveShieldsContentSettings = BraveShieldsContentSettings.getInstance();
        mBraveShieldsContentSettings.addObserver(mBraveShieldsContentSettingsObserver);

        // Growser-271: what completeRewardsInitialization() did when rewards was
        // unsupported, which is now always.
        if (mShieldsLayout != null) {
            updateShieldsLayoutBackground(true);
            mShieldsLayout.setVisibility(View.VISIBLE);
        }
    }

    public void setTabModelSelector(TabModelSelector selector) {
        // We might miss events before calling setTabModelSelector, so we need
        // to proactively update the shields button state here, otherwise shields
        // might sometimes show as disabled while it is actually enabled.
        updateBraveShieldsButtonState(getToolbarDataProvider().getTab());
        // After an activity recreation that reparents the current tab (e.g. the theme switch
        // applied on YouTube PiP exit), the page is preserved without any fresh navigation,
        // page-load, tab-selection, or tab-shown callback, so nothing would re-evaluate the PiP
        // icon and it would stay hidden until a manual reload. The tab model is (re)bound here on
        // every activity start, so proactively evaluate the current tab's icon.
        final Tab pipCurrentTab = getToolbarDataProvider().getTab();
        if (pipCurrentTab != null) {
            showYouTubePipIcon(pipCurrentTab);
        }
        mTabModelSelectorTabObserver =
                new TabModelSelectorTabObserver(selector) {
                    @Override
                    protected void onTabRegistered(Tab tab) {
                        super.onTabRegistered(tab);
                        if (tab.isIncognito()) {
                            showWalletIcon(false);
                        }
                    }

                    @Override
                    public void onCrash(Tab tab) {
                        super.onCrash(tab);
                        // When a tab crashes it shows a custom view with a reload button.
                        // The PIP layout must be hidden.
                        hideYouTubePipIcon();
                    }

                    @Override
                    public void onShown(Tab tab, @TabSelectionType int type) {
                        if (!PictureInPicture.isEnabled(getContext())) {
                            hideYouTubePipIcon();
                        }
                        // Update shields button state when visible tab is changed.
                        updateBraveShieldsButtonState(tab);
                        // case when window.open is triggered from dapps site and new tab is in
                        // focus
                        if (type != TabSelectionType.FROM_USER) {
                            dismissWalletPanelOrDialog();
                        }
                        findMediaFiles();
                    }

                    @Override
                    public void onHidden(Tab tab, @TabHidingType int reason) {
                        hidePlaylistButton();
                    }

                    @Override
                    public void onPageLoadStarted(Tab tab, GURL url) {
                        hideYouTubePipIcon();
                        showWalletIcon(false, tab);
                        if (getToolbarDataProvider().getTab() == tab) {
                            updateBraveShieldsButtonState(tab);
                        }
                        mUnifiedPanelHandler.clearBraveShieldsCount(tab.getId());
                        dismissShieldsTooltip();
                        hidePlaylistButton();
                    }

                    @Override
                    public void onPageLoadFinished(final Tab tab, GURL url) {
                        if (getToolbarDataProvider().getTab() == tab) {
                            updateBraveShieldsButtonState(tab);

                            if (mBraveShieldsButton != null
                                    && mBraveShieldsButton.isShown()
                                    && mUnifiedPanelHandler != null
                                    && !mUnifiedPanelHandler.isShowing()) {
                                checkForTooltip(tab);
                            }

                            // Re-check PiP icon visibility now that the document
                            // load is complete. The earlier check in
                            // onDidFinishNavigationInPrimaryMainFrame may have
                            // returned false because
                            // IsDocumentOnLoadCompletedInPrimaryMainFrame was not
                            // yet true at navigation commit time.
                            showYouTubePipIcon(tab);
                        }

                        String countryCode = Locale.getDefault().getCountry();
                        if (countryCode.equals(BraveConstants.INDIA_COUNTRY_CODE)
                                && url.domainIs(BraveConstants.YOUTUBE_DOMAIN)
                                && ChromeSharedPreferences.getInstance()
                                        .readBoolean(
                                                BravePreferenceKeys.BRAVE_AD_FREE_CALLOUT_DIALOG,
                                                true)) {
                            ChromeSharedPreferences.getInstance()
                                    .writeBoolean(BravePreferenceKeys.BRAVE_OPENED_YOUTUBE, true);
                        }
                    }

                    @Override
                    public void onDidFinishNavigationInPrimaryMainFrame(
                            Tab tab, NavigationHandle navigation) {
                        if (navigation.isInPrimaryMainFrame() && navigation.hasCommitted()) {
                            showYouTubePipIcon(tab);
                        } else {
                            hideYouTubePipIcon();
                        }
                        hidePlaylistButton();
                    }

                    @Override
                    public void onDestroyed(Tab tab) {
                        mUnifiedPanelHandler.removeStat(tab.getId());
                        mTabsWithWalletIcon.remove(tab.getId());
                    }
                };

        mTabModelSelectorTabModelObserver =
                new TabModelSelectorTabModelObserver(selector) {
                    @Override
                    public void didSelectTab(Tab tab, @TabSelectionType int type, int lastId) {
                        showYouTubePipIcon(tab);
                        // Growser-271: the publisher mark and the rewards tab notification
                        // (which also carried the wallet icon, #275) are gone.
                    }
                };
    }

    private void showYouTubePipIcon(@NonNull final Tab tab) {
        // The layout could be null in Custom Tabs layout.
        if (mYouTubePipLayout == null) {
            return;
        }

        // Return early if picture in picture is not supported
        // or disabled in the OS settings.
        if (!PictureInPicture.isEnabled(getContext())) {
            if (mYouTubePipLayout.getVisibility() != View.GONE) {
                mYouTubePipLayout.setVisibility(View.GONE);
                invalidateToolbarSnapshotOnTablet();
            }
            return;
        }

        // Hide the layout if the current tab is in a state where it doesn't support or allow PiP
        // mode. This can also happen when a tab is re-selected after a crash and it's showing
        // the crash custom view, or is in a frozen state (likely inactive or unloaded).
        final boolean available =
                BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                        tab.getWebContents());
        final int newVisibility = available ? View.VISIBLE : View.GONE;
        if (mYouTubePipLayout.getVisibility() != newVisibility) {
            mYouTubePipLayout.setVisibility(newVisibility);
            invalidateToolbarSnapshotOnTablet();
        }
    }

    private void hideYouTubePipIcon() {
        // The layout could be null in Custom Tabs layout.
        if (mYouTubePipLayout == null) {
            return;
        }
        if (mYouTubePipLayout.getVisibility() != View.GONE) {
            mYouTubePipLayout.setVisibility(View.GONE);
            invalidateToolbarSnapshotOnTablet();
        }
    }

    /**
     * Invalidates old toolbar bitmap snapshot on tablets, so when scrolling it will be shown the
     * right toolbar preview containing the visibility of the icons.
     */
    private void invalidateToolbarSnapshotOnTablet() {
        // mToolbarSnapshotCaptureRunnable is available on tablets only.
        if (!isAttachedToWindow() || mToolbarSnapshotCaptureRunnable == null) {
            return;
        }

        removeCallbacks(mToolbarSnapshotCaptureRunnable);
        post(mToolbarSnapshotCaptureRunnable);
    }

    private void requestToolbarSnapshotCapture() {
        final ToolbarControlContainer toolbarControlContainer =
                getRootView().findViewById(R.id.control_container);
        if (toolbarControlContainer == null) {
            return;
        }

        final ViewResourceAdapter adapter = toolbarControlContainer.getToolbarResourceAdapter();
        if (adapter == null) {
            return;
        }
        adapter.invalidate(null);
        adapter.triggerBitmapCapture();
    }

    private void hidePlaylistButton() {
        // Growser-273: the button and its layout went with the playlist.
        mShouldShowPlaylistMenu = false;
    }

    private void findMediaFiles() {
        // Growser-273: nothing collects media files any more.
    }

    // Growser-273: the playlist button, its options sheet and its snack bars
    // went with the feature.

    private void checkForTooltip(Tab tab) {
        try {
            if (BraveShieldsUtils.isTooltipShown || BraveActivity.getBraveActivity().mIsDeepLink) {
                return;
            }
            if (!BraveShieldsUtils.hasShieldsTooltipShown(BraveShieldsUtils.PREF_SHIELDS_TOOLTIP)
                    && mUnifiedPanelHandler.getTrackersBlockedCount(tab.getId())
                                    + mUnifiedPanelHandler.getAdsBlockedCount(tab.getId())
                            > 0) {
                showTooltip(BraveShieldsUtils.PREF_SHIELDS_TOOLTIP, tab.getId());
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "checkForTooltip " + e);
        }
    }

    private void showTooltip(String tooltipPref, int tabId) {
        int gravity =
                BottomToolbarConfiguration.isToolbarBottomAnchored() ? Gravity.TOP : Gravity.BOTTOM;
        mShieldsPopupWindowTooltip =
                new PopupWindowTooltip.Builder(getContext())
                        .anchorView(mBraveShieldsButton)
                        .arrowColor(
                                ContextCompat.getColor(getContext(), R.color.primitive_primary_35))
                        .gravity(gravity)
                        .dismissOnOutsideTouch(true)
                        .dismissOnInsideTouch(false)
                        .backgroundDimDisabled(false)
                        .dimAmount(0.2f)
                        .padding(0f)
                        .parentPaddingHorizontal(dpToPx(getContext(), 10))
                        .modal(true)
                        .contentView(R.layout.brave_shields_tooltip_layout)
                        .build();

        int adsTrackersCount =
                mUnifiedPanelHandler.getTrackersBlockedCount(tabId)
                        + mUnifiedPanelHandler.getAdsBlockedCount(tabId);

        TextView tvBlocked = mShieldsPopupWindowTooltip.findViewById(R.id.tv_blocked);
        tvBlocked.setText(
                String.format(
                        getContext().getResources().getString(R.string.shield_tracker_blocked),
                        String.valueOf(adsTrackersCount)));

        if (mBraveShieldsButton != null && mBraveShieldsButton.isShown()) {
            mShieldsPopupWindowTooltip.show();
            BraveShieldsUtils.setShieldsTooltipShown(tooltipPref, true);
            BraveShieldsUtils.isTooltipShown = true;
        }
    }

    public void dismissShieldsTooltip() {
        if (mShieldsPopupWindowTooltip != null && mShieldsPopupWindowTooltip.isShowing()) {
            mShieldsPopupWindowTooltip.dismiss();
            mShieldsPopupWindowTooltip = null;
        }
    }

    public void reopenShieldsPanel() {
        if (mUnifiedPanelHandler != null && mUnifiedPanelHandler.isShowing()) {
            mUnifiedPanelHandler.hide();
            showShieldsMenu();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Context context = getContext();
        if (context instanceof Activity
                && (((Activity) context).isFinishing() || ((Activity) context).isDestroyed())) {
            return;
        }
        dismissShieldsTooltip();
        reopenShieldsPanel();
    }

    private void addSavedBandwidthToDb(long savings) {
        new AsyncTask<Void>() {
            @Override
            protected Void doInBackground() {
                try {
                    SavedBandwidthTable savedBandwidthTable =
                            new SavedBandwidthTable(
                                    savings, BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                    long unused_rowId = mDatabaseHelper.insertSavedBandwidth(savedBandwidthTable);
                } catch (Exception e) {
                    // Do nothing if url is invalid.
                    // Just return w/o showing shields popup.
                    return null;
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private void addStatsToDb(String statType, String statSite, String url) {
        new AsyncTask<Void>() {
            @Override
            protected Void doInBackground() {
                try {
                    URL urlObject = new URL(url);
                    URL siteObject = new URL(statSite);
                    BraveStatsTable braveStatsTable =
                            new BraveStatsTable(
                                    url,
                                    urlObject.getHost(),
                                    statType,
                                    statSite,
                                    siteObject.getHost(),
                                    BraveStatsUtil.getCalculatedDate("yyyy-MM-dd", 0));
                    long unused_rowId = mDatabaseHelper.insertStats(braveStatsTable);
                } catch (Exception e) {
                    // Do nothing if url is invalid.
                    // Just return w/o showing shields popup.
                    return null;
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                assert ThreadUtils.runningOnUiThread();
                if (isCancelled()) return;
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public boolean isWalletIconVisible() {
        if (mWalletLayout == null) {
            return false;
        }
        return mWalletLayout.getVisibility() == View.VISIBLE;
    }

    public void showWalletIcon(boolean show, Tab tab) {
        // The layout could be null in Custom Tabs layout
        if (mWalletLayout == null) {
            return;
        }
        Tab currentTab = tab;
        if (currentTab == null) {
            currentTab = getToolbarDataProvider().getTab();
            if (currentTab == null) {
                return;
            }
        }
        // Growser-275: the wallet is out of the product, so its icon never shows.
        mWalletLayout.setVisibility(View.GONE);
        mTabsWithWalletIcon.remove(currentTab.getId());
    }

    public void showWalletIcon(boolean show) {
        showWalletIcon(show, null);
    }

    @Override
    public void onClickImpl(View v) {
        if (mUnifiedPanelHandler == null) {
            assert false;
            return;
        }
        if (mBraveShieldsButton == v && mBraveShieldsButton != null) {
            showShieldsMenu();
        } else if (mBraveWalletButton == v && mBraveWalletButton != null) {
            // Growser-271/272: no rewards button, and the home button no longer tells
            // Brave News where the user came from.
            maybeShowWalletPanel();
        } else if (mYouTubePipButton == v && mYouTubePipButton != null) {
            Tab currentTab = getToolbarDataProvider().getTab();
            if (currentTab != null
                    && BraveYouTubeScriptInjectorNativeHelper.isPictureInPictureAvailable(
                            currentTab.getWebContents())) {
                if (!PictureInPicture.isEnabled(getContext())) {
                    hideYouTubePipIcon();
                    return;
                }
                BraveYouTubeScriptInjectorNativeHelper.setFullscreen(currentTab.getWebContents());
            }
        }
    }

    private void maybeShowWalletPanel() {
        // Growser-275: there is no wallet panel.
    }

    public void showWalletPanel() {
        // Growser-275: there is no wallet panel to show.
    }

    @Override
    public void onClick(View v) {
        onClickImpl(v);
    }

    private void showShieldsMenu() {
        Tab currentTab = getToolbarDataProvider().getTab();
        if (currentTab == null) {
            return;
        }
        try {
            URL url = new URL(currentTab.getUrl().getSpec());
            if (!isValidProtocolForShields(url.getProtocol())) {
                return;
            }
            mUnifiedPanelHandler.show(this, currentTab);
        } catch (Exception e) {
            return;
        }
    }

    @Override
    public boolean onLongClickImpl(View v) {
        // Use null as the default description since Toast.showAnchoredToast
        // will return false if it is null.
        String description = null;
        Context context = getContext();
        Resources resources = context.getResources();

        if (v == mBraveShieldsButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_brave_shields);
        } else if (v == mHomeButton) { // Growser-271: no rewards button
            description = resources.getString(R.string.accessibility_toolbar_btn_home);
        } else if (v == mBraveWalletButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_brave_wallet);
        } else if (v == mYouTubePipButton) {
            description = resources.getString(R.string.accessibility_toolbar_btn_brave_pip);
        }

        return Toast.showAnchoredToast(context, v, description);
    }

    @Override
    public boolean onLongClick(View v) {
        return onLongClickImpl(v);
    }

    @Override
    public void populateUrlAnimatorSetImpl(
            boolean showExpandedState,
            int urlFocusToolbarButtonsDuration,
            int urlClearFocusTabStackDelayMs,
            List<Animator> animators) {
        if (mBraveShieldsButton != null) {
            Animator animator;
            if (showExpandedState) {
                float density = getContext().getResources().getDisplayMetrics().density;
                boolean isRtl = getLayoutDirection() == LAYOUT_DIRECTION_RTL;
                float toolbarButtonTranslationX =
                        MathUtils.flipSignIf(URL_FOCUS_TOOLBAR_BUTTONS_TRANSLATION_X_DP, isRtl)
                        * density;
                animator = ObjectAnimator.ofFloat(
                        mBraveShieldsButton, TRANSLATION_X, toolbarButtonTranslationX);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setInterpolator(Interpolators.FAST_OUT_LINEAR_IN_INTERPOLATOR);
                animators.add(animator);

                animator = ObjectAnimator.ofFloat(mBraveShieldsButton, ALPHA, 0);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setInterpolator(Interpolators.FAST_OUT_LINEAR_IN_INTERPOLATOR);
                animators.add(animator);
            } else {
                animator = ObjectAnimator.ofFloat(mBraveShieldsButton, TRANSLATION_X, 0);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setStartDelay(urlClearFocusTabStackDelayMs);
                animator.setInterpolator(Interpolators.FAST_OUT_SLOW_IN_INTERPOLATOR);
                animators.add(animator);

                animator = ObjectAnimator.ofFloat(mBraveShieldsButton, ALPHA, 1);
                animator.setDuration(urlFocusToolbarButtonsDuration);
                animator.setStartDelay(urlClearFocusTabStackDelayMs);
                animator.setInterpolator(Interpolators.FAST_OUT_SLOW_IN_INTERPOLATOR);
                animators.add(animator);
            }
        }
    }

    @Override
    public void updateModernLocationBarColorImpl(int color) {
        mCurrentToolbarColor = color;
        // Inside the location bar the buttons have no background of their own to tint.
        if (mBraveButtonsInLocationBar) {
            return;
        }
        if (mShieldsLayout != null) {
            mShieldsLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
        if (mWalletLayout != null) {
            mWalletLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
        if (mYouTubePipLayout != null) {
            mYouTubePipLayout.getBackground().setColorFilter(color, PorterDuff.Mode.SRC_IN);
        }
    }

    /**
     * If |tab| is null, set disabled image to shields button and |urlString| is ignored. If
     * |urlString| is null, url is fetched from |tab|.
     */
    public void updateBraveShieldsButtonState(Tab tab) {
        if (mBraveShieldsButton == null) {
            assert false;
            return;
        }

        if (tab == null) {
            // Growser-266: Brave Shields keeps its own brand (#55), as on the desktop.
            mBraveShieldsButton.setImageResource(
                    R.drawable.ic_social_brave_monochrome_favicon_fullheight_color);
            return;
        }
        mBraveShieldsButton.setImageResource(
                isShieldsOnForTab(tab)
                        ? R.drawable.ic_social_brave_release_favicon_fullheight_color
                        : R.drawable.ic_social_brave_monochrome_favicon_fullheight_color);
        // Growser-271: no rewards button to show or hide beside it.
    }

    private boolean isShieldsOnForTab(Tab tab) {
        if (!isNativeLibraryReady()
                || tab == null
                || Profile.fromWebContents(tab.getWebContents()) == null) {
            return false;
        }

        return BraveShieldsContentSettings.getShields(
                Profile.fromWebContents(tab.getWebContents()),
                tab.getUrl().getSpec(),
                BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);
    }

    private boolean isValidProtocolForShields(String protocol) {
        if (protocol.equals("http") || protocol.equals("https")) {
            return true;
        }

        return false;
    }

    public void dismissWalletPanelOrDialog() {
        // Growser-275: there is no wallet panel to dismiss.
    }

    public boolean isShieldsTooltipShown() {
        if (mShieldsPopupWindowTooltip != null) {
            return mShieldsPopupWindowTooltip.isShowing();
        }
        return false;
    }

    private boolean isCustomTab() {
        return BraveReflectionUtil.equalTypes(this.getClass(), CustomTabToolbar.class);
    }

    @Override
    public void onThemeColorChanged(int color, boolean shouldAnimate) {
        // Shields and rewards are brand-colored and stay untinted.
        ColorStateList tint =
                ColorUtils.shouldUseLightForegroundOnBackground(color)
                        ? mLightModeTint
                        : mDarkModeTint;
        if (mWalletIcon != null) {
            ImageViewCompat.setImageTintList(mWalletIcon, tint);
        }
        if (mYouTubePipButton != null) {
            ImageViewCompat.setImageTintList(mYouTubePipButton, tint);
        }

        final int textBoxColor =
                ThemeUtils.getTextBoxColorForToolbarBackgroundInNonNativePage(
                        getContext(), color, isIncognito(), isCustomTab());
        updateModernLocationBarColorImpl(textBoxColor);
    }

    public void onBottomControlsVisibilityChanged(boolean isVisible) {
        mIsBottomControlsVisible = isVisible;
        // The tab switcher and menu buttons are only Brave's to move between the top toolbar and
        // the bottom while Brave's own bottom controls carry them. Upstream's bottom bar carries
        // them instead, and ToolbarPhone hides the top ones for it, so showing them back here -
        // which this does whenever the omnibox takes focus - would leave a second pair on top.
        if (BottomToolbarConfiguration.isAndroidBottomBarEnabled()) {
            return;
        }
        if (BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)
                && getMenuButtonCoordinator() != null) {
            getMenuButtonCoordinator().setVisibility(!isVisible);
            ToggleTabStackButton toggleTabStackButton = findViewById(R.id.tab_switcher_button);
            if (toggleTabStackButton != null) {
                toggleTabStackButton.setVisibility(
                        isTabSwitcherOnBottomControls() ? GONE : VISIBLE);
            }
        }
    }

    /**
     * Anchors the Brave button row at the trailing end of the tablet location bar, so that the
     * focus ring upstream draws around the text box encloses the shields and rewards buttons as
     * well. The row is inflated as a child of the location bar (see toolbar_tablet.xml), which is a
     * ConstraintLayout, so it has no usable constraints until they are set here.
     */
    private void maybeAnchorBraveButtonsInLocationBar() {
        if (!BraveReflectionUtil.equalTypes(this.getClass(), ToolbarTablet.class)) {
            return;
        }

        View braveButtons = findViewById(R.id.brave_toolbar_container);
        View marginSpacer = findViewById(R.id.margin_spacer);
        if (braveButtons == null || marginSpacer == null) {
            return;
        }
        ConstraintLayout.LayoutParams spacerParams =
                (ConstraintLayout.LayoutParams) marginSpacer.getLayoutParams();

        Resources resources = getResources();
        ConstraintLayout.LayoutParams params =
                new ConstraintLayout.LayoutParams(
                        ConstraintLayout.LayoutParams.WRAP_CONTENT,
                        resources.getDimensionPixelSize(R.dimen.modern_toolbar_background_size));
        params.endToEnd = ConstraintLayout.LayoutParams.PARENT_ID;
        params.topToTop = ConstraintLayout.LayoutParams.PARENT_ID;
        params.bottomToBottom = ConstraintLayout.LayoutParams.PARENT_ID;
        params.setMarginEnd(
                resources.getDimensionPixelSize(R.dimen.location_bar_url_action_offset));
        braveButtons.setLayoutParams(params);

        // Every upstream action button chain ends at |margin_spacer|, and the barriers that keep
        // the URL text clear of those buttons reference it, so re-anchoring it ahead of the Brave
        // row is enough to make room for the row.
        spacerParams.startToEnd = ConstraintLayout.LayoutParams.UNSET;
        spacerParams.endToStart = braveButtons.getId();
        marginSpacer.setLayoutParams(spacerParams);

        mBraveButtonsInLocationBar = true;
        // The location bar paints the text box behind the buttons now, so the segment drawables
        // that continue it on phones would only double up here.
        for (View layout :
                new View[] {mYouTubePipLayout, mWalletLayout, mShieldsLayout}) { // Growser-271
            if (layout != null) {
                layout.setBackground(null);
            }
        }
    }

    private void updateShieldsLayoutBackground(boolean rounded) {
        if (mShieldsLayout == null || mBraveButtonsInLocationBar) {
            return;
        }

        mShieldsLayout.setBackgroundDrawable(
                ApiCompatibilityUtils.getDrawable(getContext().getResources(),
                        rounded ? R.drawable.modern_toolbar_background_grey_end_segment
                                : R.drawable.modern_toolbar_background_grey_middle_segment));

        updateModernLocationBarColorImpl(mCurrentToolbarColor);
    }

    private boolean isTabSwitcherOnBottomControls() {
        return mIsBottomControlsVisible
                && BottomToolbarVariationManager.isTabSwitcherOnBottomControls();
    }

    private boolean isMenuButtonOnBottomControls() {
        return mIsBottomControlsVisible
                && BottomToolbarVariationManager.isMenuButtonOnBottomControls();
    }

    @Override
    public void initialize(
            ToolbarDataProvider toolbarDataProvider,
            ToolbarTabController tabController,
            MenuButtonCoordinator menuButtonCoordinator,
            @Nullable ToggleTabStackButtonCoordinator tabSwitcherButtonCoordinator,
            HistoryDelegate historyDelegate,
            UserEducationHelper userEducationHelper,
            MonotonicObservableSupplier<Tracker> trackerSupplier,
            ToolbarProgressBar progressBar,
            @Nullable ReloadButtonCoordinator reloadButtonCoordinator,
            @Nullable BackButtonCoordinator backButtonCoordinator,
            @Nullable ForwardButtonCoordinator forwardButtonCoordinator,
            HomeButtonCoordinator homeButtonCoordinator,
            @Nullable SigninButtonCoordinator signinButtonCoordinator,
            ThemeColorProvider themeColorProvider,
            IncognitoStateProvider incognitoStateProvider,
            @Nullable Supplier<Integer> incognitoWindowCountSupplier,
            WindowAndroid windowAndroid) {
        super.initialize(
                toolbarDataProvider,
                tabController,
                menuButtonCoordinator,
                tabSwitcherButtonCoordinator,
                historyDelegate,
                userEducationHelper,
                trackerSupplier,
                progressBar,
                reloadButtonCoordinator,
                backButtonCoordinator,
                forwardButtonCoordinator,
                homeButtonCoordinator,
                signinButtonCoordinator,
                themeColorProvider,
                incognitoStateProvider,
                incognitoWindowCountSupplier,
                windowAndroid);

        BraveMenuButtonCoordinator.setMenuFromBottom(
                isMenuButtonOnBottomControls() || isMenuOnBottomWithBottomAddressBar());
    }

    public void updateWalletBadgeVisibility(boolean visible) {
        assert mBraveWalletBadge != null;
        mBraveWalletBadge.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void updateMenuButtonState() {
        if (BottomToolbarConfiguration.isBraveBottomControlsEnabled()) {
            BraveMenuButtonCoordinator.setMenuFromBottom(mIsBottomControlsVisible);
        } else {
            BraveMenuButtonCoordinator.setMenuFromBottom(isMenuOnBottomWithBottomAddressBar());
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (BraveReflectionUtil.equalTypes(this.getClass(), CustomTabToolbar.class)
                || BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)) {
            updateMenuButtonState();
            Tab tab = getToolbarDataProvider() != null ? getToolbarDataProvider().getTab() : null;
            if (tab != null && tab.getWebContents() != null) {
                updateBraveShieldsButtonState(tab);
            }
        }
        super.onDraw(canvas);
    }

    @Override
    public boolean isLocationBarValid(LocationBarCoordinator locationBar) {
        return locationBar != null && locationBar.getPhoneCoordinator() != null
                && locationBar.getPhoneCoordinator().getViewForDrawing() != null;
    }

    @Override
    public void drawAnimationOverlay(ViewGroup toolbarButtonsContainer, Canvas canvas) {
        if (mWalletLayout != null && mWalletLayout.getVisibility() != View.GONE) {
            canvas.save();
            ViewUtils.translateCanvasToView(toolbarButtonsContainer, mWalletLayout, canvas);
            mWalletLayout.draw(canvas);
            canvas.restore();
        }
        if (mShieldsLayout != null && mShieldsLayout.getVisibility() != View.GONE) {
            canvas.save();
            ViewUtils.translateCanvasToView(toolbarButtonsContainer, mShieldsLayout, canvas);
            mShieldsLayout.draw(canvas);
            canvas.restore();
        }
        if (mYouTubePipLayout != null && mYouTubePipLayout.getVisibility() != View.GONE) {
            canvas.save();
            ViewUtils.translateCanvasToView(toolbarButtonsContainer, mYouTubePipLayout, canvas);
            mYouTubePipLayout.draw(canvas);
            canvas.restore();
        }
    }

    private boolean isMenuOnBottomWithBottomAddressBar() {
        // If address bar is not on bottom, then menu is not on bottom too.
        if (!BottomToolbarConfiguration.isToolbarBottomAnchored()) {
            return false;
        }
        // Menu can be on bottom only with ToolbarPhone.
        if (!BraveReflectionUtil.equalTypes(this.getClass(), ToolbarPhone.class)) {
            return false;
        }
        // In overview mode the menu is on top.
        Context context = getContext();
        if (context instanceof BraveActivity && ((BraveActivity) context).isInOverviewMode()) {
            return false;
        }
        return true;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        maybeHideTopTabSwitcherButton();

        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        // Growser-271: there is no rewards layout to hide.
    }

    /**
     * Re-enforces GONE on the top toolbar's tab switcher button when Brave's tab switcher lives on
     * the bottom toolbar. ToolbarPhone.updateButtonVisibility() (and related upstream paths) calls
     * setHasSpaceToShow(true) on the tab switcher coordinator, which forces the button VISIBLE and
     * overrides the GONE state set in onBottomControlsVisibilityChanged.
     */
    private void maybeHideTopTabSwitcherButton() {
        if (!isTabSwitcherOnBottomControls()) {
            return;
        }
        View toggleTabStackButton = findViewById(R.id.tab_switcher_button);
        if (toggleTabStackButton != null && toggleTabStackButton.getVisibility() != GONE) {
            toggleTabStackButton.setVisibility(GONE);
        }
    }

}
