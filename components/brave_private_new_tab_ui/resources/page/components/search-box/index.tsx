// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'
import { getLocale } from '$web-common/locale'

const Box = styled.div`
  display: flex;
  justify-content: center;
  flex-direction: column;
  align-items: center;
  gap: 40px;
  width: 100%;
`

const Form = styled.form`
  --bg-color: rgba(255, 255, 255, 0.22);
  --box-shadow: 0px 2px 70px rgba(0, 0, 0, 0.3);

  display: grid;
  grid-template-columns: 1fr 50px;
  align-items: center;
  width: 100%;
  height: 52px;
  font-family: apple-system, BlinkMacSystemFont, "Segoe UI", "Helvetica Neue", Roboto, Oxygen, Ubuntu, Cantarell, "Fira Sans", "Droid Sans", sans-serif;
  color: white;
  font-size: 14px;
  font-weight: 400;
  background: var(--bg-color);
  border-radius: 8px;
  transition: box-shadow 0.3s ease-in-out;
  overflow: hidden;

  &:focus-within,
  &:hover {
    box-shadow: var(--box-shadow);
  }

  input[type="text"] {
    width: 100%;
    height: 36px;
    border: 0;
    background-color: transparent;
    padding: 5px 16px;

    &:focus {
      outline: 0;
    }

    &::placeholder {
      color: rgba(255,255,255,0.7);
    }
  }
`

const IconButton = styled.button`
  background: transparent;
  padding: 0;
  margin: 0;
  border: 0;
  width: 100%;
  height: 100%;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;

  &:hover {
    background: linear-gradient(304.74deg, #6F4CD2 15.81%, #BF14A2 63.17%, #F73A1C 100%);

    path {
      fill: white;
    }
  }
`

// growser: логотип приватной вкладки — зелёное кольцо+G (белое кольцо для
// контраста на тёмном фоне) + wordmark «Growser» (зелёная G, светлые буквы).
function BraveSearchLogo () {
  return (
    <svg xmlns="http://www.w3.org/2000/svg" width="248" height="64" fill="none" aria-label="Growser">
      <g transform="scale(0.0625)">
        <path fill="#FFFFFF" fillRule="evenodd" clipRule="evenodd" d="M 993.28 512.00 C 993.28 246.20 777.80 30.72 512.00 30.72 C 246.20 30.72 30.72 246.20 30.72 512.00 C 30.72 777.80 246.20 993.28 512.00 993.28 C 777.80 993.28 993.28 777.80 993.28 512.00 Z M 909.42 512.00 C 909.42 731.49 731.49 909.42 512.00 909.42 C 292.51 909.42 114.58 731.49 114.58 512.00 C 114.58 292.51 292.51 114.58 512.00 114.58 C 731.49 114.58 909.42 292.51 909.42 512.00 Z"/>
        <path fill="#1EA362" d="M 557.19 492.96 L 785.42 492.96 L 785.42 511.28 C 785.42 552.79 780.53 589.49 770.77 621.38 C 761.23 650.99 745.13 678.65 722.46 704.37 C 671.19 762.13 605.98 791.01 526.85 791.01 C 449.56 791.01 383.36 763.09 328.27 707.23 C 273.25 651.22 245.74 583.99 245.74 505.55 C 245.74 425.43 273.75 357.48 329.76 301.70 C 385.76 245.62 453.94 217.57 534.29 217.57 C 577.48 217.57 617.81 226.39 655.28 244.01 C 690.99 261.72 726.17 290.33 760.81 329.86 L 701.40 386.74 C 656.08 326.46 600.87 296.32 535.78 296.32 C 477.33 296.32 428.34 316.47 388.82 356.76 C 349.21 396.36 329.41 445.96 329.41 505.55 C 329.41 567.05 351.46 617.72 395.57 657.55 C 436.85 694.49 481.57 712.95 529.72 712.95 C 570.69 712.95 607.51 699.14 640.17 671.52 C 672.90 643.59 691.18 610.21 694.99 571.37 L 557.19 571.37 L 557.19 492.96 Z"/>
      </g>
      <text x="78" y="46" fontFamily="-apple-system, 'Helvetica Neue', Arial, sans-serif" fontSize="40" fontWeight="700" letterSpacing="-1.5">
        <tspan fill="#1EA362">G</tspan><tspan fill="#F3F5F7">rowser</tspan>
      </text>
    </svg>
  )
}

interface Props {
  onSubmit?: (value: string, openNewTab: boolean) => unknown
}

function Search (props: Props) {
  const [value, setValue] = React.useState('')
  const inputRef = React.useRef<HTMLInputElement>(null)

  const onInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    setValue(e.currentTarget.value)
  }

  const handleFormBoxClick = () => {
    inputRef.current && inputRef.current.focus()
  }

  const handleSubmit = (e: React.ChangeEvent<HTMLFormElement>) => {
    e.preventDefault()
    props.onSubmit?.(value, false)
  }

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (value === '') return

    if ((e.metaKey || e.ctrlKey) && (e.key === 'Enter')) {
      props.onSubmit?.(value, true)
    }
  }

  return (
    <Box>
      <BraveSearchLogo />
      <Form onSubmit={handleSubmit} onClick={handleFormBoxClick} role="search" aria-label="Growser">
        <input ref={inputRef} onChange={onInputChange} onKeyDown={handleKeyDown} type="text" placeholder={getLocale('searchPlaceholderLabel')} value={value} autoCapitalize="off" autoComplete="off" autoCorrect="off" spellCheck="false" aria-label="Search" title="Search" aria-autocomplete="none" aria-haspopup="false" maxLength={2048} autoFocus />
        <IconButton data-testid="submit_button" aria-label="Submit">
          <svg width="20" height="20" fill="none" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M8 16a8 8 0 1 1 5.965-2.67l5.775 5.28a.8.8 0 1 1-1.08 1.18l-5.88-5.375A7.965 7.965 0 0 1 8 16Zm4.374-3.328a.802.802 0 0 0-.201.18 6.4 6.4 0 1 1 .202-.181Z" fill="url(#search_icon_gr)"/><defs><linearGradient id="search_icon_gr" x1="20" y1="20" x2="-2.294" y2="3.834" gradientUnits="userSpaceOnUse"><stop stopColor="#BF14A2"/><stop offset="1" stopColor="#F73A1C"/></linearGradient></defs></svg>
        </IconButton>
      </Form>
    </Box>
  )
}

export default Search
