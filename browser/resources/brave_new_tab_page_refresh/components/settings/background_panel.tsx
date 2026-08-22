/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import Toggle from '@brave/leo/react/toggle'

import {
  useBackgroundState,
  useBackgroundActions,
} from '../../context/background_context'
import { getString } from '../../lib/strings'
import { inlineCSSVars } from '../../lib/inline_css_vars'
import { BackgroundTypePanel } from './background_type_panel'
import { SettingsPanel } from './settings_panel'
import classnames from '$web-common/classnames'

import {
  SelectedBackgroundType,
  backgroundCSSValue,
  gradientPreviewBackground,
  solidPreviewBackground,
} from '../../state/background_store'

import { style } from './background_panel.style'

export function BackgroundPanel() {
  const actions = useBackgroundActions()

  const backgroundsEnabled = useBackgroundState((s) => s.backgroundsEnabled)
  const backgroundsCustomizable = useBackgroundState(
    (s) => s.backgroundsCustomizable,
  )
  const selectedBackground = useBackgroundState((s) => s.selectedBackground)
  const braveBackgrounds = useBackgroundState((s) => s.braveBackgrounds)
  const customBackgrounds = useBackgroundState((s) => s.customBackgrounds)

  const [panelType, setPanelType] =
    React.useState<SelectedBackgroundType | null>(null)
  const [uploading, setUploading] = React.useState(false)

  React.useEffect(() => {
    setUploading(false)
  }, [selectedBackground, customBackgrounds])

  function getTypePreviewValue(type: SelectedBackgroundType) {
    const isSelectedType = type === selectedBackground.type
    switch (type) {
      case SelectedBackgroundType.kBrave:
        return braveBackgrounds[0]?.imageUrl ?? ''
      case SelectedBackgroundType.kCustom:
        if (isSelectedType && selectedBackground.value) {
          return selectedBackground.value
        }
        return customBackgrounds[0] ?? ''
      case SelectedBackgroundType.kSolid:
        if (isSelectedType && selectedBackground.value) {
          return selectedBackground.value
        }
        return solidPreviewBackground
      case SelectedBackgroundType.kGradient:
        if (isSelectedType && selectedBackground.value) {
          return selectedBackground.value
        }
        return gradientPreviewBackground
      default:
        console.error('Unhandled background type', type)
        return ''
    }
  }

  function renderUploadPreview() {
    return (
      <div className='preview upload'>
        {uploading ? <ProgressRing /> : <Icon name='upload' />}
        {getString(S.NEW_TAB_UPLOAD_BACKGROUND_LABEL)}
      </div>
    )
  }

  function renderTypePreview(type: SelectedBackgroundType) {
    if (
      type === SelectedBackgroundType.kCustom
      && customBackgrounds.length === 0
    ) {
      return renderUploadPreview()
    }
    const selected = type === selectedBackground.type
    return (
      <div
        className={classnames({ preview: true, selected })}
        style={inlineCSSVars({
          '--preview-background': backgroundCSSValue(
            type,
            getTypePreviewValue(type),
          ),
        })}
      >
        {selected && (
          <span className='selected-marker'>
            <Icon name='check-normal' />
          </span>
        )}
      </div>
    )
  }

  function showCustomBackgroundChooser() {
    actions.showCustomBackgroundChooser().then((backgroundSelected) => {
      if (backgroundSelected) {
        setUploading(true)
      }
    })
  }

  function onCustomPreviewClick() {
    if (customBackgrounds.length === 0) {
      showCustomBackgroundChooser()
    } else {
      setPanelType(SelectedBackgroundType.kCustom)
    }
  }

  function subPanelTitle(type: SelectedBackgroundType) {
    switch (type) {
      case SelectedBackgroundType.kCustom:
        return getString(S.NEW_TAB_CUSTOM_BACKGROUND_LABEL)
      case SelectedBackgroundType.kGradient:
        return getString(S.NEW_TAB_GRADIENT_BACKGROUND_LABEL)
      case SelectedBackgroundType.kSolid:
        return getString(S.NEW_TAB_SOLID_BACKGROUND_LABEL)
      default:
        return ''
    }
  }

  if (panelType !== null) {
    return (
      <SettingsPanel
        cssScope={style.scope}
        title={subPanelTitle(panelType)}
        onBack={() => setPanelType(null)}
      >
        <BackgroundTypePanel
          backgroundType={panelType}
          renderUploadOption={() => (
            <button onClick={showCustomBackgroundChooser}>
              {renderUploadPreview()}
            </button>
          )}
        />
      </SettingsPanel>
    )
  }

  return (
    <SettingsPanel
      cssScope={style.scope}
      title={getString(S.NEW_TAB_BACKGROUND_SETTINGS_TITLE)}
    >
      <Toggle
        className='toggle-row'
        size='small'
        checked={backgroundsEnabled}
        onChange={({ checked }) => {
          actions.setBackgroundsEnabled(checked)
        }}
      >
        <span className='label'>
          {getString(S.NEW_TAB_SHOW_BACKGROUNDS_LABEL)}
        </span>
      </Toggle>
      {/* growser (#78): no "show sponsored images" switch. Its whole branch is
          behind rewardsFeatureEnabled, and Rewards is compiled out
          (ENABLE_BRAVE_REWARDS=0), so the row never rendered - it only carried a
          support.brave.app link into the bundle. Same treatment as the
          sponsored-sites switch in the top sites panel. */}
      {backgroundsEnabled && backgroundsCustomizable && (
        <>
          <div className='background-options'>
            <div className='background-option'>
              <button
                onClick={() => {
                  actions.selectBackground(SelectedBackgroundType.kBrave, '')
                }}
              >
                {renderTypePreview(SelectedBackgroundType.kBrave)}
                {getString(S.NEW_TAB_BRAVE_BACKGROUND_LABEL)}
              </button>
            </div>
            <div className='background-option'>
              <button onClick={onCustomPreviewClick}>
                {renderTypePreview(SelectedBackgroundType.kCustom)}
                {getString(S.NEW_TAB_CUSTOM_BACKGROUND_LABEL)}
              </button>
            </div>
            <div className='background-option'>
              <button
                onClick={() => setPanelType(SelectedBackgroundType.kSolid)}
              >
                {renderTypePreview(SelectedBackgroundType.kSolid)}
                {getString(S.NEW_TAB_SOLID_BACKGROUND_LABEL)}
              </button>
            </div>
            <div className='background-option'>
              <button
                onClick={() => setPanelType(SelectedBackgroundType.kGradient)}
              >
                {renderTypePreview(SelectedBackgroundType.kGradient)}
                {getString(S.NEW_TAB_GRADIENT_BACKGROUND_LABEL)}
              </button>
            </div>
          </div>
        </>
      )}
    </SettingsPanel>
  )
}
