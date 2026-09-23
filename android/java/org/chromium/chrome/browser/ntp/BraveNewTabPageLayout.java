/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.build.NullUtil.assertNonNull;
import static org.chromium.build.NullUtil.assumeNonNull;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Display;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.ImageView;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.OnItemTouchListener;
import androidx.recyclerview.widget.SimpleItemAnimator;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.AsyncTask;
import org.chromium.build.annotations.EnsuresNonNull;
import org.chromium.build.annotations.Initializer;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.model.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.model.Wallpaper;
import org.chromium.chrome.browser.ntp_background_images.util.FetchWallpaperWorkerTask;
import org.chromium.chrome.browser.ntp_background_images.util.NTPImageUtil;
import org.chromium.chrome.browser.onboarding.OnboardingPrefManager;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.settings.BackgroundImagesPreferences;
import org.chromium.chrome.browser.suggestions.tile.BraveMostVisitedTilesLayoutBase;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAttributes;
import org.chromium.chrome.browser.tabmodel.TabModelSelector;
import org.chromium.chrome.browser.tabmodel.TabModelUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.ui.base.WindowAndroid;

import java.util.function.Supplier;

@SuppressWarnings("UseSharedPreferencesManagerFromChromeCheck")
@NullMarked
// Growser-272/305: no news controller, and no stats card to listen to.
public class BraveNewTabPageLayout extends NewTabPageLayout {
    private static final String TAG = "BraveNewTabPage";

    private @Nullable ViewGroup mMvTilesContainerLayout;

    // Own members.
    private WindowAndroid mWindowAndroid;

    private @Nullable ImageView mBgImageView;
    private @Nullable SponsoredRichMediaWebView mSponsoredRichMediaWebView;
    private @Nullable FrameLayout mBackgroundSponsoredRichMediaView;

    // Own members
    private @Nullable Profile mProfile;

    private @Nullable SponsoredTab mSponsoredTab;
    private @Nullable BitmapDrawable mImageDrawable;

    private @Nullable FetchWallpaperWorkerTask mWorkerTask;
    private boolean mIsFromBottomSheet;
    private @Nullable NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;

    private @Nullable Tab mTab;
    private Activity mActivity;

    private @Nullable BraveNtpAdapter mNtpAdapter;
    private @Nullable Bitmap mSponsoredLogo;
    private @Nullable Wallpaper mWallpaper;

    private @Nullable BraveNewTabTakeoverInfobar mNewTabTakeoverInfobar;

    private @Nullable RecyclerView mRecyclerView;

    private @Nullable NTPImage mNtpImageGlobal;

    private SharedPreferences.@Nullable OnSharedPreferenceChangeListener mPreferenceListener;
    private boolean mIsTopSitesEnabled;
    private ViewTreeObserver.@Nullable OnGlobalLayoutListener mBgImageViewOnGlobalLayoutListener;

    private @Nullable NewTabTakeoverSafeAreaReporter mSafeAreaReporter;

    public BraveNewTabPageLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    protected void updateTileGridPlaceholderVisibility() {
        // This function is kept empty to avoid placeholder implementation
    }

    @EnsuresNonNull({"mMvTilesContainerLayout"})
    protected void initializeSiteSectionView() {
        mMvTilesContainerLayout =
                (ViewGroup) ((ViewStub) findViewById(R.id.mv_tiles_layout_stub)).inflate();
        mMvTilesContainerLayout.setPadding(0, 0, 0, 0);
        mMvTilesContainerLayout.setVisibility(View.VISIBLE);

        mMvTilesContainerLayout.post(
                new Runnable() {
                    @Override
                    public void run() {
                        assumeNonNull(mMvTilesContainerLayout);
                        mMvTilesContainerLayout.addOnLayoutChangeListener(
                                (View view,
                                        int left,
                                        int top,
                                        int right,
                                        int bottom,
                                        int oldLeft,
                                        int oldTop,
                                        int oldRight,
                                        int oldBottom) -> {
                                    int oldHeight = oldBottom - oldTop;
                                    int newHeight = bottom - top;

                                    if (oldHeight != newHeight
                                            && mIsTopSitesEnabled
                                            && mNtpAdapter != null) {
                                        new Handler(Looper.getMainLooper())
                                                .post(
                                                        () -> {
                                                            assertNonNull(mNtpAdapter);
                                                            mNtpAdapter.notifyItemRangeChanged(
                                                                    0, 2); // Growser-272/305
                                                        });
                                    }
                                });
                    }
                });

        // The page contents are initially hidden; otherwise they'll be drawn centered on the
        // page before the tiles are available and then jump upwards to make space once the
        // tiles are available.
        if (getVisibility() != View.VISIBLE) setVisibility(View.VISIBLE);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        if (mSponsoredTab == null) {
            initilizeSponsoredTab();
        }
        getAndShowNTPImage();

        if (OnboardingPrefManager.getInstance().isFromNotification()) {
            ((BraveActivity) mActivity).showOnboardingV2(false);
            OnboardingPrefManager.getInstance().setFromNotification(false);
        }

        initPreferenceObserver();
        if (mPreferenceListener != null) {
            ContextUtils.getAppSharedPreferences()
                    .registerOnSharedPreferenceChangeListener(mPreferenceListener);
        }
        setNtpViews();

        // Show recent tabs dialog for variants B, C, and D if NTP was shown after inactivity
        maybeShowRecentTabsDialog();
    }

    private void setNtpViews() {
        // Growser-272: the news settings bar and "new content" pill left with Brave
        // News.
        mRecyclerView = findViewById(R.id.recyclerview);
        // Growser-272: the news package's wrapper went with it.
        LinearLayoutManager linearLayoutManager =
                new LinearLayoutManager(mActivity, LinearLayoutManager.VERTICAL, false);
        mRecyclerView.setLayoutManager(linearLayoutManager);
        mRecyclerView.post(
                new Runnable() {
                    @Override
                    public void run() {
                        setNtpRecyclerView(linearLayoutManager);
                    }
                });
    }

    private void setNtpRecyclerView(LinearLayoutManager linearLayoutManager) {
        assertNonNull(mRecyclerView);

        mIsTopSitesEnabled = NtpUtil.shouldDisplayTopSites();

        if (mNtpAdapter == null) {
            if (mActivity != null && !mActivity.isDestroyed() && !mActivity.isFinishing()) {
                // Growser-272: no feed to hand the adapter.
                mNtpAdapter = new BraveNtpAdapter(mActivity, mMvTilesContainerLayout,
                        mNtpImageGlobal, mSponsoredTab, mWallpaper, mSponsoredLogo,
                        mNTPBackgroundImagesBridge, mRecyclerView.getHeight(),
                        mIsTopSitesEnabled);

                mRecyclerView.setAdapter(mNtpAdapter);

                if (mRecyclerView.getItemAnimator() != null) {
                    RecyclerView.ItemAnimator itemAnimator = mRecyclerView.getItemAnimator();
                    if (itemAnimator instanceof SimpleItemAnimator) {
                        SimpleItemAnimator simpleItemAnimator = (SimpleItemAnimator) itemAnimator;
                        simpleItemAnimator.setSupportsChangeAnimations(false);
                    }
                }
            }
        } else {
            mNtpAdapter.setRecyclerViewHeight(mRecyclerView.getHeight());
            mNtpAdapter.setTopSitesEnabled(mIsTopSitesEnabled);
        }

        if (mNtpAdapter == null) return;

        keepPosition(); // Growser-272: no feed to load first

        assertNonNull(mRecyclerView);
        mRecyclerView.addOnItemTouchListener(
                new OnItemTouchListener() {
                    @Override
                    public boolean onInterceptTouchEvent(
                            RecyclerView recyclerView, MotionEvent event) {
                        final View childView =
                                recyclerView.findChildViewUnder(event.getX(), event.getY());
                        if (childView == null && mSponsoredRichMediaWebView != null) {
                            mSponsoredRichMediaWebView.getView().dispatchTouchEvent(event);
                        }
                        return false;
                    }

                    @Override
                    public void onTouchEvent(RecyclerView recyclerView, MotionEvent event) {}

                    @Override
                    public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept) {}
                });

        mRecyclerView.addOnScrollListener(
                new RecyclerView.OnScrollListener() {
                    @Override
                    public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
                        super.onScrollStateChanged(recyclerView, newState);
                        int tabId = -1;
                        try {
                            Tab tab = BraveActivity.getBraveActivity().getActivityTab();
                            tabId = tab != null ? tab.getId() : -1;
                        } catch (BraveActivity.BraveActivityNotFoundException e) {
                            Log.e(TAG, "onScrollStateChanged " + e);
                        }
                        if (newState == RecyclerView.SCROLL_STATE_IDLE) {
                            assertNonNull(mRecyclerView);
                            if (tabId != -1 && mRecyclerView.getChildCount() > 0) {
                                View firstChild = mRecyclerView.getChildAt(0);
                                if (firstChild != null) {
                                    int firstVisiblePosition =
                                            mRecyclerView.getChildAdapterPosition(firstChild);
                                    int verticalOffset = firstChild.getTop();

                                    ChromeSharedPreferences.getInstance()
                                            .writeInt(
                                                    BravePreferenceKeys
                                                                    .BRAVE_RECYCLERVIEW_OFFSET_POSITION // presubmit: ignore-long-line
                                                            + tabId,
                                                    verticalOffset);

                                    ChromeSharedPreferences.getInstance()
                                            .writeInt(
                                                    BravePreferenceKeys.BRAVE_RECYCLERVIEW_POSITION
                                                            + tabId,
                                                    firstVisiblePosition);
                                }
                            }
                        }
                        // Growser-272: the feed's view counting and "new content"
                        // check left with Brave News; onScrolled only served the feed.
                    }
                });

        maybeCreateSafeAreaReporter();
    }

    private void keepPosition() {
        try {
            Tab tab = BraveActivity.getBraveActivity().getActivityTab();
            if (tab != null) {
                int itemPosition =
                        ChromeSharedPreferences.getInstance()
                                .readInt(
                                        BravePreferenceKeys.BRAVE_RECYCLERVIEW_POSITION
                                                + tab.getId(),
                                        0);

                new Handler(Looper.getMainLooper())
                        .postDelayed(
                                () -> {
                                    assertNonNull(mRecyclerView);
                                    if (mNtpAdapter != null
                                            && mNtpAdapter.getItemCount() > itemPosition) {
                                        RecyclerView.LayoutManager manager =
                                                mRecyclerView.getLayoutManager();
                                        if (manager instanceof LinearLayoutManager) {
                                            int offsetPosition =
                                                    ChromeSharedPreferences.getInstance()
                                                            .readInt(
                                                                    BravePreferenceKeys
                                                                                    .BRAVE_RECYCLERVIEW_OFFSET_POSITION // presubmit: ignore-long-line
                                                                            + tab.getId(),
                                                                    0);

                                            if (itemPosition == mNtpAdapter.getTopSitesCount()) {
                                                offsetPosition -=
                                                        mNtpAdapter.getTopMarginImageCredit();
                                            }

                                            LinearLayoutManager linearLayoutManager =
                                                    (LinearLayoutManager) manager;
                                            linearLayoutManager.scrollToPositionWithOffset(
                                                    itemPosition, offsetPosition);
                                        }
                                    }
                                },
                                10);
            }
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "keepPosition " + e);
        }
    }

    private void initPreferenceObserver() {
        mPreferenceListener =
                (prefs, key) -> {
                    // Growser-272: the news source and show-news keys left with Brave
                    // News.
                    if (TextUtils.equals(
                            key, BackgroundImagesPreferences.PREF_SHOW_TOP_SITES)) {
                        assertNonNull(mNtpAdapter);
                        mIsTopSitesEnabled = NtpUtil.shouldDisplayTopSites();
                        mNtpAdapter.setTopSitesEnabled(mIsTopSitesEnabled);
                    }
                };
    }

    @Override
    protected void onDetachedFromWindow() {
        assertNonNull(mRecyclerView);

        if (mWorkerTask != null && mWorkerTask.getStatus() == AsyncTask.Status.RUNNING) {
            mWorkerTask.cancel(true);
            mWorkerTask = null;
        }

        var observer =
                mBgImageView != null && mBgImageViewOnGlobalLayoutListener != null
                        ? mBgImageView.getViewTreeObserver()
                        : null;
        if (observer != null && observer.isAlive()) {
            observer.removeOnGlobalLayoutListener(mBgImageViewOnGlobalLayoutListener);
            mBgImageViewOnGlobalLayoutListener = null;
        }

        if (!mIsFromBottomSheet) {
            setBackgroundResource(0);
            if (mImageDrawable != null && mImageDrawable.getBitmap() != null
                    && !mImageDrawable.getBitmap().isRecycled()) {
                mImageDrawable.getBitmap().recycle();
            }
        }

        // Growser-272: no feed to keep and no news controller to close.

        maybeResetSponsoredRichMediaBackground();

        // Removes preference listener.
        ContextUtils.getAppSharedPreferences()
                .unregisterOnSharedPreferenceChangeListener(mPreferenceListener);
        mPreferenceListener = null;

        mRecyclerView.clearOnScrollListeners();
        super.onDetachedFromWindow();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (mActivity != null && (mActivity.isFinishing() || mActivity.isDestroyed())) {
            return;
        }

        if (mSponsoredTab == null || !NTPImageUtil.shouldEnableNTPFeature()) {
            return;
        }

        // `maybeShowNTPImage()` is a no-op if configuration changes before the NTP image is ready;
        // it will be called again once NTP image is ready.
        maybeShowNTPImage();

        new Handler(Looper.getMainLooper())
                .postDelayed(
                        () -> {
                            assertNonNull(mRecyclerView);
                            if (mNtpAdapter != null) {
                                mNtpAdapter.setRecyclerViewHeight(mRecyclerView.getHeight());
                            }
                            keepPosition();
                        },
                        10);
    }

    @Initializer
    public void initialize(
            NewTabPageManager manager,
            Activity activity,
            Profile profile,
            WindowAndroid windowAndroid) {

        mProfile = profile;

        mNTPBackgroundImagesBridge = NTPBackgroundImagesBridge.getInstance(mProfile);
        mWindowAndroid = windowAndroid;

        assert mMvTilesContainerLayout != null : "Something has changed in the upstream!";

        if (mMvTilesContainerLayout != null && useFixedMVTLayout()) {
            ViewGroup tilesLayout = mMvTilesContainerLayout.findViewById(R.id.mv_tiles_layout);

            assert tilesLayout instanceof BraveMostVisitedTilesLayoutBase
                    : "Something has changed in the upstream!";

            if (tilesLayout instanceof BraveMostVisitedTilesLayoutBase) {
                ((BraveMostVisitedTilesLayoutBase) tilesLayout).setUseFixedLayout(true);
            }
        }

        assert (activity instanceof BraveActivity);
        mActivity = activity;
        ((BraveActivity) mActivity).dismissShieldsTooltip();
        ((BraveActivity) mActivity).setNewTabPageManager(manager);
        // Growser-272: Brave News started its controller here, on every NTP.
    }

    protected boolean useFixedMVTLayout() {
        return !UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE);
    }

    public void setTabProvider(Supplier<@Nullable Tab> unused_tabProvider) {}

    private void maybeShowNTPImage() {
        // Return early if the NTP image is not available yet; this method is called again once it
        // is ready.
        if (mNtpImageGlobal == null) {
            return;
        }

        Display display = mActivity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        if (mNtpAdapter != null) {
            mNtpAdapter.setNtpImage(mNtpImageGlobal);
        }

        boolean wasWallpaperShown = true;
        if (mNtpImageGlobal instanceof Wallpaper && ((Wallpaper) mNtpImageGlobal).isRichMedia()) {
            setupSponsoredBackgroundContent((Wallpaper) mNtpImageGlobal);
        } else {
            maybeResetSponsoredRichMediaBackground();

            if (UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                    && mSponsoredTab != null
                    && NTPImageUtil.shouldEnableNTPFeature()) {
                setBackgroundImage(mNtpImageGlobal);
            } else {
                wasWallpaperShown = false;
            }
        }

        assertNonNull(mProfile);
        if (wasWallpaperShown
                && mNtpImageGlobal instanceof Wallpaper
                && getTab() != null
                && mNewTabTakeoverInfobar == null) {
            mNewTabTakeoverInfobar = new BraveNewTabTakeoverInfobar(mProfile);
            mNewTabTakeoverInfobar.maybeDisplayAndIncrementCounter(mActivity, getTab());
        }
    }

    private void setupSponsoredBackgroundContent(Wallpaper wallpaper) {
        if (mSponsoredRichMediaWebView == null) {
            mSponsoredRichMediaWebView =
                    new SponsoredRichMediaWebView(mActivity, mWindowAndroid, mProfile);

            mBackgroundSponsoredRichMediaView = findViewById(R.id.bg_sponsored_rich_media_view);
            mBackgroundSponsoredRichMediaView.setVisibility(View.VISIBLE);
            mBackgroundSponsoredRichMediaView.addView(mSponsoredRichMediaWebView.getView());
        }

        mSponsoredRichMediaWebView.maybeLoadSponsoredRichMedia(
                wallpaper.getWallpaperId(), wallpaper.getCreativeInstanceId());

        maybeCreateSafeAreaReporter();
    }

    private void maybeResetSponsoredRichMediaBackground() {
        if (mBackgroundSponsoredRichMediaView == null || mSponsoredRichMediaWebView == null) {
            return;
        }

        destroySafeAreaReporter();

        mBackgroundSponsoredRichMediaView.setVisibility(View.GONE);
        mBackgroundSponsoredRichMediaView.removeAllViews();
        mSponsoredRichMediaWebView.destroy();
        mSponsoredRichMediaWebView = null;
    }

    private void maybeCreateSafeAreaReporter() {
        if (mRecyclerView == null
                || mNtpAdapter == null
                || mSponsoredRichMediaWebView == null
                || mBackgroundSponsoredRichMediaView == null) {
            return;
        }

        if (mSafeAreaReporter != null) {
            mSafeAreaReporter.scheduleMeasurement();
            return;
        }

        mSafeAreaReporter =
                new NewTabTakeoverSafeAreaReporter(
                        this,
                        mRecyclerView,
                        mNtpAdapter,
                        mBackgroundSponsoredRichMediaView,
                        mSponsoredRichMediaWebView);
    }

    private void destroySafeAreaReporter() {
        if (mSafeAreaReporter != null) {
            mSafeAreaReporter.destroy();
            mSafeAreaReporter = null;
        }
    }

    private void setBackgroundImage(NTPImage ntpImage) {
        mBgImageView = (ImageView) findViewById(R.id.bg_image_view);
        mBgImageView.setScaleType(ImageView.ScaleType.MATRIX);

        if (mBgImageViewOnGlobalLayoutListener == null) {
            mBgImageViewOnGlobalLayoutListener =
                    new ViewTreeObserver.OnGlobalLayoutListener() {
                        private int mLastWidth;
                        private int mLastHeight;

                        @Override
                        public void onGlobalLayout() {
                            assertNonNull(mBgImageView);
                            int currentWidth = mBgImageView.getMeasuredWidth();
                            int currentHeight = mBgImageView.getMeasuredHeight();

                            // Only re-fetch if dimensions actually changed
                            if (currentWidth > 0
                                    && currentHeight > 0
                                    && (currentWidth != mLastWidth
                                            || currentHeight != mLastHeight)) {
                                mLastWidth = currentWidth;
                                mLastHeight = currentHeight;

                                if (mWorkerTask != null) {
                                    mWorkerTask.cancel(true);
                                }

                                mWorkerTask =
                                        new FetchWallpaperWorkerTask(
                                                ntpImage,
                                                currentWidth,
                                                currentHeight,
                                                mWallpaperRetrievedCallback);
                                mWorkerTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                            }
                        }
                    };
        }
        ViewTreeObserver observer = mBgImageView.getViewTreeObserver();
        observer.addOnGlobalLayoutListener(mBgImageViewOnGlobalLayoutListener);
    }

    private void getAndShowNTPImage() {
        assertNonNull(mSponsoredTab);
        mSponsoredTab.getNTPImage(
                /* allowSponsoredImage= */ true,
                ntpImage -> {
                    if (mActivity == null || mActivity.isFinishing() || mActivity.isDestroyed()) {
                        return;
                    }
                    mNtpImageGlobal = ntpImage;
                    maybeShowNTPImage();
                });
    }

    private void initilizeSponsoredTab() {
        if (TabAttributes.from(getTab()).get(String.valueOf(getTab().getId())) == null) {
            SponsoredTab sponsoredTab =
                    new SponsoredTab(mNTPBackgroundImagesBridge, /* allowSponsoredImage= */ true);
            TabAttributes.from(getTab()).set(String.valueOf(getTab().getId()), sponsoredTab);
        }
        mSponsoredTab = TabAttributes.from(getTab()).get(String.valueOf(getTab().getId()));
    }

    private final FetchWallpaperWorkerTask.WallpaperRetrievedCallback mWallpaperRetrievedCallback =
            new FetchWallpaperWorkerTask.WallpaperRetrievedCallback() {
                @Override
                public void bgWallpaperRetrieved(Bitmap bgWallpaper) {
                    if (mBgImageView != null) {
                        mBgImageView.setImageBitmap(bgWallpaper);
                    }
                }

                @Override
                public void logoRetrieved(Wallpaper wallpaper, Bitmap logoWallpaper) {
                    mWallpaper = wallpaper;
                    mSponsoredLogo = logoWallpaper;
                    if (mNtpAdapter != null) {
                        mNtpAdapter.setSponsoredLogo(mWallpaper, logoWallpaper);
                    }
                }
            };

    public void setTab(Tab tab) {
        mTab = tab;
    }

    private Tab getTab() {
        assert mTab != null;
        return mTab;
    }

    /**
     * Shows the recent tabs snackbar if variant B is active, and either OPTION_NEW_TAB or
     * OPTION_NEW_TAB_AFTER_INACTIVITY is selected. For OPTION_NEW_TAB_AFTER_INACTIVITY, only shows
     * when app was returned from background after inactivity threshold.
     */
    private void maybeShowRecentTabsDialog() {
        if (!BraveFreshNtpHelper.isEnabled() || !BraveFreshNtpHelper.getVariant().equals("B")) {
            return;
        }

        // Check if OPTION_NEW_TAB or OPTION_NEW_TAB_AFTER_INACTIVITY is selected
        int openingScreenOption =
                ChromeSharedPreferences.getInstance()
                        .readInt(BravePreferenceKeys.BRAVE_NEW_TAB_PAGE_OPENING_SCREEN, 1);
        if (openingScreenOption
                        != BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB_AFTER_INACTIVITY
                && openingScreenOption != BravePreferenceKeys.BRAVE_OPENING_SCREEN_OPTION_NEW_TAB) {
            return;
        }

        // Check the flag set by BraveReturnToChromeUtil when NTP was shown
        // For OPTION_NEW_TAB_AFTER_INACTIVITY, flag is set only after inactivity threshold
        // For OPTION_NEW_TAB, flag is set whenever NTP is shown at startup
        boolean shouldShowSnackbar =
                ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, false);
        if (!shouldShowSnackbar) {
            return;
        }

        // Clear the flag so snackbar is only shown once
        ChromeSharedPreferences.getInstance()
                .writeBoolean(BravePreferenceKeys.BRAVE_SHOW_RECENT_TABS_SNACKBAR, false);

        // Wait for tab state to be initialized before showing snackbar.
        // On cold start, tabs are restored asynchronously, so we need to wait
        // for them to be fully loaded before searching for the last active tab.
        BraveActivity braveActivity = (BraveActivity) mActivity;
        TabModelSelector tabModelSelector = braveActivity.getTabModelSelectorSupplier().get();
        assertNonNull(tabModelSelector);
        TabModelUtils.runOnTabStateInitialized(
                tabModelSelector,
                (selector) -> {
                    BraveRecentTabsSnackbarHelper snackbarHelper =
                            new BraveRecentTabsSnackbarHelper();
                    snackbarHelper.showSnackbar(braveActivity);
                });
    }
}
