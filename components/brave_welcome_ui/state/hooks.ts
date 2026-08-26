// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { BrowserProfile, ImportDataBrowserProxyImpl } from '../api/welcome_browser_proxy'
import { loadTimeData } from '$web-common/loadTimeData'
import { BrowserType, ViewType } from './component_types'
import DataContext from './context'

const browserList = Object.values(BrowserType)

export const getValidBrowserProfiles = (profiles: BrowserProfile[]) => {
  const getBrowserName = (toFind: string) => {
    // TODO(tali): Add exact matching for cases like "Chrome" vs "Chrome Canary"
    return browserList.find(browser => toFind.includes(browser))
  }

  let results = profiles
    .filter((profile) => profile.name !== 'Bookmarks HTML File')
    .map((profile) => {
      const browserType = getBrowserName(profile.name)
      // Introducing a new property here
      return { ...profile, browserType }
    })

  return results
}

export function useInitializeImportData () {
  const [browserProfiles, setProfiles] = React.useState<BrowserProfile[] | undefined>(undefined)

  React.useEffect(() => {
    const fetchAllBrowserProfiles = async () => {
      const res = await ImportDataBrowserProxyImpl.getInstance().initializeImportDialog()
      const validProfiles = getValidBrowserProfiles(res)
      setProfiles(validProfiles)
    }

    fetchAllBrowserProfiles()
  }, [])

  return {
    browserProfiles
  }
}

export function useProfileCount () {
  const profileCountRef = React.useRef(0)

  const incrementCount = () => {
    profileCountRef.current++
  }

  const decrementCount = () => {
    profileCountRef.current--
  }

  return {
    profileCountRef,
    incrementCount,
    decrementCount
  }
}

export const shouldPlayAnimations = loadTimeData.getBoolean('hardwareAccelerationEnabledAtStartup') &&
    !window.matchMedia('(prefers-reduced-motion: reduce)').matches

// This hook is a kind of finite state machine that helps transition between view types.
// It's intended to put transition logic in one place, so that we can easily understand
// what's going on and add or remove a state from the graph.
// Returns three transition functions: forward(), back() and skip().
interface ViewTypeState {
  forward: ViewType;
  back?: ViewType;
  skip?: ViewType;
  fail?: ViewType;
}

export function useViewTypeTransition(currentViewType: ViewType | undefined) : ViewTypeState {
  const { browserProfiles, currentSelectedBrowserProfiles} = React.useContext(DataContext)

  const states = React.useMemo(() => {
    // growser: the WDP (#24) and Help Improve (#21) screens are out of the flow.
    // Web Discovery is pointless with Yandex as the search default, and the Help
    // Improve screen existed to opt the user into P3A and metrics reporting -
    // it initialised both toggles to true and wrote them on Finish, which
    // quietly undid our off-by-default prefs (#39). After import we go straight
    // to ImportSucceeded (SetupComplete), which shows the check mark and opens
    // the welcome-complete URL.
    const nextAfterImport = ViewType.ImportSucceeded

    return {
      [ViewType.DefaultBrowser]: {  // The initial state view
        forward: !browserProfiles || browserProfiles.length === 0 ?
            ViewType.ImportSelectTheme : ViewType.ImportSelectBrowser
      },
      [ViewType.ImportSelectTheme]: {
        forward: nextAfterImport
      },
      [ViewType.ImportSelectBrowser]: {
        forward: currentSelectedBrowserProfiles &&
            currentSelectedBrowserProfiles.length > 1 ?
            ViewType.ImportSelectProfile : ViewType.ImportInProgress,
        skip: nextAfterImport,
      },
      [ViewType.ImportSelectProfile]: {
        forward: ViewType.ImportInProgress,
        back: ViewType.ImportSelectBrowser
      },
      [ViewType.ImportInProgress]: {
        forward: ViewType.ImportSucceeded,
        fail: ViewType.ImportFailed,
      },
      // ImportSucceeded is terminal (a self-loop): SetupComplete opens the
      // welcome-complete URL itself when the Lottie finishes, and never uses forward.
      [ViewType.ImportSucceeded]: { forward: ViewType.ImportSucceeded },
      [ViewType.ImportFailed]: {
        forward: nextAfterImport
      },
    }
  }, [browserProfiles, currentSelectedBrowserProfiles])

  return states[currentViewType?? ViewType.DefaultBrowser]
}
