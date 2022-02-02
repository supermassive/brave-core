/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import * as style from './promo_mobile.style'
import { CloseStrokeIcon } from 'brave-ui/components/icons'
import { LocaleContext } from '../../../../shared/lib/locale_context'

export interface Props {
  title: string
  imagePath: string
  link: string
  copy: JSX.Element
  disclaimer?: string | JSX.Element
  onDismissPromo: (event: React.MouseEvent<HTMLDivElement>) => void
}

export default function PromoMobile (props: Props) {
  const { getString } = React.useContext(LocaleContext)

  function onLearnMore () {
    window.open(props.link, '_blank', 'noopener')
  }

  const {
    copy,
    title,
    imagePath,
    disclaimer,
    onDismissPromo
  } = props

  return (
    <style.root>
      <style.closeIcon onClick={onDismissPromo}>
        <CloseStrokeIcon />
      </style.closeIcon>
      <style.content>
        <style.title>
          {title}
        </style.title>
        <style.copy>
          {copy}
        </style.copy>
        {
          disclaimer
          ? <style.disclaimer>
            {disclaimer}
          </style.disclaimer>
          : null
        }
        <style.learnMoreButton onClick={onLearnMore}>
          {getString('promoLearnMore')}
        </style.learnMoreButton>
        <style.dismissButton onClick={onDismissPromo}>
          {getString('promoDismiss')}
        </style.dismissButton>
        <style.background src={imagePath} />
      </style.content>
    </style.root>
  )
}
