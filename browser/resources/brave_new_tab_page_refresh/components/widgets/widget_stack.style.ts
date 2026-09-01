/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { color } from '@brave/leo/tokens/css/variables'
import { scoped } from '$web-common/scoped_css'

export const style = scoped.css`
  & {
    flex-grow: 1;
    display: flex;
    align-items: stretch;
  }

  .stack-tabs {
    --leo-icon-size: 16px;
    --leo-icon-color: ${color.icon.default};

    display: flex;
    flex-direction: column;
    /* growser (#139): 12px is the ceiling for corners in this product. */
    border-radius: 12px 0 0 12px;
    overflow: clip;

    > * {
      background: rgba(255, 255, 255, 0.10);
      flex: 1 1 auto;
      padding: 0 16px;
      display: flex;
      align-items: center;
    }

    .active {
      --leo-icon-color: #fff;
      background: inherit;
    }
  }

  .widget {
    flex: 1 1 auto;
    padding: 16px;
    display: flex;
    flex-direction: column;
  }
`
