/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  src?: string
}

export const root = styled('div')<{}>`
  max-width: 373px;
  width: 373px;
  height: 338px;
  overflow: hidden;
  margin-bottom: 12px;
`

export const closeIcon = styled('div')<{}>`
  width: 12px;
  height: 12px;
  color: #fff;
  float: right;
  margin: 11px 11px 0 0;
`

export const background = styled('div')<StyleProps>`
  width: 320px;
  height: 100px;
  background: url(${p => p.src}) no-repeat;
  border-radius: 4px;
`

export const content = styled('div')<{}>`
  background: linear-gradient(125.83deg, #392DD1 0%, #22B8CF 99.09%);
  max-width: 100%;
  width: 100%;
  height: 338px;
  padding: 18px 26px 12px 26px;
  border-radius: 4px;
`

export const copy = styled('div')<{}>`
  font-family: Poppins, sans-serif;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: #fff;
  width: 320px;
  height: 54px;
  text-align: center;
  margin-bottom: 24px;
`

export const title = styled('span')<{}>`
  font-family: Poppins, sans-serif;
  font-weight: 600;
  font-size: 20px;
  width: 320px;
  line-height: 26px;
  letter-spacing: 0.15px;
  color: #fff;
  display: block;
  margin-bottom: 9px;
  text-align: center;
`

export const disclaimer = styled('span')<{}>`
  display: block;
  margin: 10px 0px;
  font-size: 11px;
  font-weight: bold;
  color: ${palette.grey500};
  font-family: Muli, sans-serif;
`

export const learnMoreButton = styled('div')<{}>`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  padding: 6px 18px;
  width: 320px;
  height: 30px;
  color: #4C54D2;
  background: #fff;
  border-radius: 48px;
  margin-bottom: 11px;
  font-family: Poppins;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
`

export const dismissButton = styled('div')<{}>`
  width: 320px;
  height: 20px;
  font-family: Poppins;
  font-size: 12px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.01em;
  color: #F0F2FF;
  margin-bottom: 23px;
`
