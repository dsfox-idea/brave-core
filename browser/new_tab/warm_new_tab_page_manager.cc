/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/warm_new_tab_page_manager.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/browser/new_tab/warm_new_tab_page_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_enums.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "components/tab_groups/tab_group_id.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

namespace growser {

BASE_FEATURE(kWarmNewTabPage,
             "GrowserWarmNewTabPage",
             base::FEATURE_ENABLED_BY_DEFAULT);

// Watches the warm contents for the signal that it is ready to be shown (its
// first visually non-empty paint). The load/stop callbacks are logged as
// diagnostics while the mechanism is being stabilized.
class WarmNewTabPageManager::PaintWatcher : public content::WebContentsObserver {
 public:
  PaintWatcher(content::WebContents* contents, base::OnceClosure on_ready)
      : content::WebContentsObserver(contents),
        on_ready_(std::move(on_ready)) {}

  PaintWatcher(const PaintWatcher&) = delete;
  PaintWatcher& operator=(const PaintWatcher&) = delete;
  ~PaintWatcher() override = default;

  // content::WebContentsObserver:
  //
  // A detached, ownerless WebContents loads fully in the background (process,
  // bundle parse, DOM, resources) but never emits DidFirstVisuallyNonEmptyPaint
  // because it has no on-screen surface to composite into. The achievable
  // "warm" signal is therefore DidStopLoading: the page's heavy work is done and
  // the final frame composites in ~one frame when the contents is adopted into a
  // visible tab. First paint still marks ready if a surface is ever attached.
  void DidStopLoading() override {
    VLOG(1) << "WarmNTP: DidStopLoading";
    RunReady();
  }

  void DidFirstVisuallyNonEmptyPaint() override {
    VLOG(1) << "WarmNTP: DidFirstVisuallyNonEmptyPaint";
    RunReady();
  }

 private:
  void RunReady() {
    if (on_ready_) {
      std::move(on_ready_).Run();
    }
  }

  base::OnceClosure on_ready_;
};

WarmNewTabPageManager::WarmNewTabPageManager(Profile* profile)
    : profile_(profile) {
  // Build off the current task so the WebContents is created after startup has
  // finished bringing up //content, not in the middle of profile creation.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&WarmNewTabPageManager::BuildWarmContents,
                                weak_factory_.GetWeakPtr()));
}

WarmNewTabPageManager::~WarmNewTabPageManager() = default;

std::unique_ptr<content::WebContents>
WarmNewTabPageManager::TakeWarmContents() {
  if (!is_ready_ || !warm_contents_) {
    return nullptr;
  }
  paint_watcher_.reset();
  is_ready_ = false;
  std::unique_ptr<content::WebContents> taken = std::move(warm_contents_);
  // Prepare a fresh warm page for the next new tab.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&WarmNewTabPageManager::BuildWarmContents,
                                weak_factory_.GetWeakPtr()));
  return taken;
}

void WarmNewTabPageManager::Rebuild() {
  BuildWarmContents();
}

void WarmNewTabPageManager::Shutdown() {
  paint_watcher_.reset();
  warm_contents_.reset();
  is_ready_ = false;
}

void WarmNewTabPageManager::BuildWarmContents() {
  is_ready_ = false;
  paint_watcher_.reset();

  const GURL url(chrome::kChromeUINewTabURL);
  content::WebContents::CreateParams params(profile_.get());
  // Visible (not initially_hidden) so resources load and the page paints in the
  // background, the way WebUIContentsPreloadManager warms top-chrome WebUIs.
  params.initially_hidden = false;
  params.site_instance = content::SiteInstance::CreateForURL(profile_.get(), url);
  warm_contents_ = content::WebContents::Create(params);

  paint_watcher_ = std::make_unique<PaintWatcher>(
      warm_contents_.get(),
      base::BindOnce(&WarmNewTabPageManager::OnWarmContentsReady,
                     weak_factory_.GetWeakPtr()));

  content::NavigationController::LoadURLParams load_params(url);
  load_params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL | ui::PAGE_TRANSITION_FROM_API);
  warm_contents_->GetController().LoadURLWithParams(load_params);

  VLOG(1) << "WarmNTP: building warm NTP for " << url;
}

void WarmNewTabPageManager::OnWarmContentsReady() {
  is_ready_ = true;
  VLOG(1) << "WarmNTP: warm NTP ready to show";
}

content::WebContents* MaybeAdoptWarmNewTab(BrowserWindowInterface* browser) {
  if (!base::FeatureList::IsEnabled(kWarmNewTabPage)) {
    return nullptr;
  }
  Browser* chrome_browser = browser->GetBrowserForMigrationOnly();
  if (!chrome_browser || !chrome_browser->SupportsWindowFeature(
                             Browser::WindowFeature::kFeatureTabStrip)) {
    return nullptr;
  }
  Profile* profile = browser->GetProfile();
  if (!profile) {
    return nullptr;
  }
  // Only adopt an already-prepared page; never create/warm here, so this stays
  // inert in headless/automation where the activator never ran.
  WarmNewTabPageManager* manager =
      WarmNewTabPageManagerFactory::GetForProfileIfExists(profile);
  if (!manager) {
    return nullptr;
  }
  std::unique_ptr<content::WebContents> warm = manager->TakeWarmContents();
  if (!warm) {
    return nullptr;
  }

  // Match chrome::NewTab: the new tab joins the active tab's group, if any.
  TabStripModel* tab_strip_model = browser->GetTabStripModel();
  const tabs::TabInterface* active_tab = tab_strip_model->GetActiveTab();
  const std::optional<tab_groups::TabGroupId> group =
      active_tab ? active_tab->GetGroup() : std::nullopt;

  content::WebContents* raw = warm.get();
  tab_strip_model->AddWebContents(std::move(warm), /*index=*/-1,
                                  ui::PAGE_TRANSITION_TYPED,
                                  AddTabTypes::ADD_ACTIVE, group);
  return raw;
}

}  // namespace growser
