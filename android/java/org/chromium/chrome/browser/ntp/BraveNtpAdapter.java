/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.app.Activity;
import android.graphics.Bitmap;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.NTPImage;
import org.chromium.chrome.browser.ntp_background_images.model.SponsoredTab;
import org.chromium.chrome.browser.ntp_background_images.model.Wallpaper;
import org.chromium.chrome.browser.ntp_background_images.util.NTPImageUtil;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.chrome.browser.util.BraveTouchUtils;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.content_public.browser.LoadUrlParams;
import org.chromium.url.Origin;

// Growser-272/305: this adapter carried Brave News' feed - the opt-in card, the
// loading row, the "new content" pill, the feed cards and the no-sources card,
// all counted into its positions - and the privacy stats card. News is out of
// the product and the stats card is off the page, as on the desktop (#136); the
// top sites and the image credit are what is left.
public class BraveNtpAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {
    private final Activity mActivity;
    private final View mMvTilesContainerLayout;
    private NTPImage mNtpImage;
    private final SponsoredTab mSponsoredTab;
    private Bitmap mSponsoredLogo;
    private Wallpaper mWallpaper;
    private final NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private boolean mIsTopSitesEnabled;
    private int mRecyclerViewHeight;
    private int mTopSitesHeight;
    private int mTopMarginImageCredit;

    private static final int TYPE_TOP_SITES = 2;
    private static final int TYPE_IMAGE_CREDIT = 4;

    private static final int ONE_ITEM_SPACE = 1;

    // Matches @dimen/mvt_container_lateral_margin used on the New Tab Page layout.
    static final int CARD_MARGIN_DP = 16;

    public BraveNtpAdapter(
            Activity activity,
            View mvTilesContainerLayout,
            NTPImage ntpImage,
            SponsoredTab sponsoredTab,
            Wallpaper wallpaper,
            Bitmap sponsoredLogo,
            NTPBackgroundImagesBridge nTPBackgroundImagesBridge,
            int recyclerViewHeight,
            boolean isTopSitesEnabled) {
        mActivity = activity;
        mMvTilesContainerLayout = mvTilesContainerLayout;
        mNtpImage = ntpImage;
        mSponsoredTab = sponsoredTab;
        mWallpaper = wallpaper;
        mSponsoredLogo = sponsoredLogo;
        mNTPBackgroundImagesBridge = nTPBackgroundImagesBridge;
        mRecyclerViewHeight = recyclerViewHeight;
        mIsTopSitesEnabled = isTopSitesEnabled;
    }

    @Override
    public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        if (holder instanceof TopSitesViewHolder) {
            LinearLayout.LayoutParams layoutParams =
                    new LinearLayout.LayoutParams(
                            LinearLayout.LayoutParams.MATCH_PARENT,
                            LinearLayout.LayoutParams.WRAP_CONTENT);
            int margin = dpToPx(mActivity, CARD_MARGIN_DP);
            layoutParams.setMargins(margin, margin, margin, 0);

            mMvTilesContainerLayout.setLayoutParams(layoutParams);
            mMvTilesContainerLayout.setBackgroundResource(R.drawable.rounded_dark_bg_alpha);
            mTopSitesHeight = NTPImageUtil.getViewHeight(holder.itemView) + margin;

        } else if (holder instanceof ImageCreditViewHolder) {
            ImageCreditViewHolder imageCreditViewHolder = (ImageCreditViewHolder) holder;

            if (UserPrefs.get(ProfileManager.getLastUsedRegularProfile())
                            .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                    && mSponsoredTab != null
                    && NTPImageUtil.shouldEnableNTPFeature()) {
                if (mNtpImage instanceof BackgroundImage) {
                    BackgroundImage backgroundImage = (BackgroundImage) mNtpImage;
                    imageCreditViewHolder.mSponsoredLogo.setVisibility(View.GONE);

                    if (backgroundImage.getImageCredit() != null) {
                        String imageCreditStr =
                                String.format(
                                        mActivity
                                                .getResources()
                                                .getString(
                                                        R.string.photo_by,
                                                        backgroundImage
                                                                .getImageCredit()
                                                                .getName()));

                        SpannableStringBuilder spannableString =
                                new SpannableStringBuilder(imageCreditStr);
                        spannableString.setSpan(
                                new android.text.style.StyleSpan(android.graphics.Typeface.BOLD),
                                ((imageCreditStr.length() - 1)
                                        - (backgroundImage.getImageCredit().getName().length()
                                                - 1)),
                                imageCreditStr.length(),
                                Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);

                        imageCreditViewHolder.mCreditTv.setText(spannableString);
                        imageCreditViewHolder.mCreditTv.setVisibility(View.VISIBLE);

                        imageCreditViewHolder.mCreditTv.setOnClickListener(
                                view -> {
                                    if (backgroundImage.getImageCredit() != null) {
                                        TabUtils.openUrlInSameTab(
                                                backgroundImage.getImageCredit().getUrl());
                                    }
                                });
                    }
                }
            }
            if (mSponsoredLogo != null) {
                imageCreditViewHolder.mSponsoredLogo.setVisibility(View.VISIBLE);
                imageCreditViewHolder.mSponsoredLogo.setImageBitmap(mSponsoredLogo);
                imageCreditViewHolder.mSponsoredLogo.setOnClickListener(
                        view -> {
                            if (mWallpaper.getLogoDestinationUrl() != null) {
                                if (mActivity instanceof BraveActivity) {
                                    // Do a renderer initiated navigation to open links
                                    // in their app/PWA (if installed).
                                    LoadUrlParams loadUrlParams =
                                            new LoadUrlParams(mWallpaper.getLogoDestinationUrl());
                                    loadUrlParams.setIsRendererInitiated(true);
                                    loadUrlParams.setHasUserGesture(true);
                                    loadUrlParams.setInitiatorOrigin(Origin.createOpaqueOrigin());
                                    ((BraveActivity) mActivity)
                                            .getActivityTab()
                                            .loadUrl(loadUrlParams);
                                }
                                mNTPBackgroundImagesBridge.wallpaperLogoClicked(mWallpaper);
                            }
                        });
            }

            if (mRecyclerViewHeight > 0) {
                LinearLayout.LayoutParams layoutParams =
                        new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                                LinearLayout.LayoutParams.WRAP_CONTENT);

                mTopMarginImageCredit =
                        mRecyclerViewHeight
                                - NTPImageUtil.getViewHeight(imageCreditViewHolder.itemView);

                mTopMarginImageCredit -= dpToPx(mActivity, CARD_MARGIN_DP);

                if (mIsTopSitesEnabled) {
                    mTopMarginImageCredit -= mTopSitesHeight;
                }

                if (mTopMarginImageCredit < 0) {
                    mTopMarginImageCredit = 0;
                }

                layoutParams.setMargins(0, mTopMarginImageCredit, 0, 0);

                imageCreditViewHolder.mNtpImageCreditLayout.setLayoutParams(layoutParams);
            }
        }
    }

    @Override
    public int getItemCount() {
        return getTopSitesCount() + ONE_ITEM_SPACE;
    }

    @NonNull
    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view;
        if (viewType == TYPE_TOP_SITES) {
            // mMvTilesContainerLayout may have been placed in the NTP layout tree by
            // initializeSiteSectionView (via ViewStub inflation). Detach it first so
            // RecyclerView can adopt it as an item view without an "already has a parent" error.
            ViewGroup existingParent = (ViewGroup) mMvTilesContainerLayout.getParent();
            if (existingParent != null) existingParent.removeView(mMvTilesContainerLayout);
            return new TopSitesViewHolder(mMvTilesContainerLayout);

        } else {
            view = LayoutInflater.from(parent.getContext())
                           .inflate(R.layout.ntp_image_credit, parent, false);
            return new ImageCreditViewHolder(view);
        }
    }

    @Override
    public int getItemViewType(int position) {
        if (getTopSitesCount() == 1 && position == 0) {
            return TYPE_TOP_SITES;
        } else {
            return TYPE_IMAGE_CREDIT;
        }
    }

    public int getTopSitesCount() {
        return mIsTopSitesEnabled ? 1 : 0;
    }

    public int getTopItemsCount() {
        return getTopSitesCount();
    }

    public void setTopSitesEnabled(boolean isTopSitesEnabled) {
        if (mIsTopSitesEnabled != isTopSitesEnabled) {
            mIsTopSitesEnabled = isTopSitesEnabled;
            if (mIsTopSitesEnabled) {
                notifyItemInserted(0);
            } else {
                notifyItemRemoved(0);
            }
            // Rebind items shifted by the insert/remove above so they're positioned correctly.
            notifyItemRangeChanged(0, getTopSitesCount() + ONE_ITEM_SPACE);
        }
    }

    public int getTopMarginImageCredit() {
        return mTopMarginImageCredit;
    }

    public void setSponsoredLogo(Wallpaper wallpaper, Bitmap sponsoredLogo) {
        mWallpaper = wallpaper;
        mSponsoredLogo = sponsoredLogo;
        notifyItemChanged(getTopSitesCount());
    }

    public void setNtpImage(NTPImage ntpImage) {
        mNtpImage = ntpImage;
        notifyItemChanged(getTopSitesCount());
    }

    public void setRecyclerViewHeight(int recyclerViewHeight) {
        mRecyclerViewHeight = recyclerViewHeight;
        notifyItemRangeChanged(0, getItemCount());
    }

    public static class TopSitesViewHolder extends RecyclerView.ViewHolder {
        TopSitesViewHolder(View itemView) {
            super(itemView);
        }
    }

    public static class ImageCreditViewHolder extends RecyclerView.ViewHolder {
        LinearLayout mNtpImageCreditLayout;
        FrameLayout mImageCreditLayout;
        TextView mCreditTv;
        ImageView mSponsoredLogo;

        ImageCreditViewHolder(View itemView) {
            super(itemView);
            this.mNtpImageCreditLayout =
                    (LinearLayout) itemView.findViewById(R.id.ntp_image_credit_layout);
            this.mImageCreditLayout = (FrameLayout) itemView.findViewById(R.id.image_credit_layout);
            this.mCreditTv = (TextView) itemView.findViewById(R.id.credit_text);
            this.mSponsoredLogo = (ImageView) itemView.findViewById(R.id.sponsored_logo);
            BraveTouchUtils.ensureMinTouchTarget(this.mCreditTv);
        }
    }
}
