/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Button from '@brave/leo/react/button'

import { formatString } from '$web-common/formatString'

import { useWelcomeApi } from '../api/welcome_api_context'
import { useStepTransition } from './use_step_transition'
import { getString } from '../lib/strings'
import { StepHeader } from './step_header'
import { ProductCard } from './product_card'

import { style } from './metrics_step.style'

import wdpImage from '../assets/wdp.svg'
import p3aImage from '../assets/p3a.svg'
import crashesImage from '../assets/crashes.svg'

interface Props {
  onBack: () => void
  onNext: () => void
}

export function MetricsStep(props: Props) {
  const api = useWelcomeApi()

  useStepTransition()

  const webDiscoveryFeatureEnabled = api.useWebDiscoveryFeatureEnabledData()
  const isWebDiscoveryPrefManaged = api.useIsWebDiscoveryPrefManagedData()
  const isCrashReportingPrefManaged = api.useIsCrashReportingPrefManagedData()

  const showWebDiscovery =
    webDiscoveryFeatureEnabled && !isWebDiscoveryPrefManaged
  // growser (#78): no P3A card. P3A is functionally removed (#21) - Init() is
  // wrapped in #if 0, so nothing is ever sent even with the pref forced on.
  // Offering someone a switch for it during onboarding asks them to make a
  // decision that has no effect. The comment in p3a_service.cc claimed the
  // onboarding opt-in was already gone; it was gone from the older welcome
  // flow, not from this one.
  const showP3A: boolean = false
  const showCrashReports = !isCrashReportingPrefManaged

  const [webDiscoveryEnabled, setWebDiscoveryEnabled] = React.useState(true)
  const [p3aEnabled, setP3AEnabled] = React.useState(true)
  const [crashReportingEnabled, setCrashReportingEnabled] = React.useState(true)

  function saveAndContinue() {
    if (showWebDiscovery) {
      api.setWebDiscoveryEnabled([webDiscoveryEnabled])
    }
    if (showP3A) {
      api.setP3AEnabled([p3aEnabled])
    }
    if (showCrashReports) {
      api.setCrashReportsEnabled([crashReportingEnabled])
    }
    props.onNext()
  }

  return (
    <div
      data-css-scope={style.scope}
      className='step-view'
    >
      <div className='step-content'>
        <div className='step-text'>
          <StepHeader />
          <h1>{getString('WELCOME_PAGE_METRICS_STEP_TITLE')}</h1>
          <p>{getString('WELCOME_PAGE_METRICS_STEP_TEXT1')}</p>
          <p>
            {formatString(getString('WELCOME_PAGE_METRICS_STEP_TEXT2'), {
              $1: (
                <a
                  href='chrome://settings/privacy'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  {/* growser (#78): the visible address is our scheme, not
                      Brave's. The href stays chrome:// because that is what
                      resolves; only the text was wrong, and it is the first
                      internal address a new user is ever shown. */}
                  growser://settings/privacy
                </a>
              ),
              $2: (content) => (
                <a
                  href='https://growser.org/features.html#privacy'
                  target='_blank'
                  rel='noopener noreferrer'
                >
                  {content}
                </a>
              ),
            })}
          </p>
        </div>
        <div className='step-ui'>
          {showWebDiscovery && (
            <ProductCard
              image={wdpImage}
              title={getString('WELCOME_PAGE_PRODUCT_WDP_TITLE')}
              description={getString('WELCOME_PAGE_PRODUCT_WDP_DESCRIPTION')}
              learnMoreUrl='https://support.brave.app/hc/articles/4409406835469-What-is-the-Web-Discovery-Project'
              checked={webDiscoveryEnabled}
              onChange={setWebDiscoveryEnabled}
            />
          )}
          {showP3A && (
            <ProductCard
              image={p3aImage}
              title={getString('WELCOME_PAGE_PRODUCT_P3A_TITLE')}
              description={getString('WELCOME_PAGE_PRODUCT_P3A_DESCRIPTION')}
              learnMoreUrl='https://support.brave.app/hc/en-us/articles/9140465918093-What-is-P3A-in-Brave-'
              checked={p3aEnabled}
              onChange={setP3AEnabled}
            />
          )}
          {showCrashReports && (
            <ProductCard
              image={crashesImage}
              title={getString('WELCOME_PAGE_PRODUCT_CRASH_REPORTS_TITLE')}
              description={getString(
                'WELCOME_PAGE_PRODUCT_CRASH_REPORTS_DESCRIPTION',
              )}
              learnMoreUrl='https://growser.org/features.html#crash-reports'
              checked={crashReportingEnabled}
              onChange={setCrashReportingEnabled}
            />
          )}
        </div>
      </div>
      <footer>
        <div className='back'>
          <Button
            kind='plain-faint'
            size='large'
            onClick={props.onBack}
          >
            {getString('WELCOME_PAGE_BACK_BUTTON_LABEL')}
          </Button>
        </div>
        <div className='forward'>
          <Button
            kind='filled'
            size='large'
            onClick={saveAndContinue}
          >
            {getString('WELCOME_PAGE_START_BROWSING_BUTTON_LABEL')}
          </Button>
        </div>
      </footer>
    </div>
  )
}
