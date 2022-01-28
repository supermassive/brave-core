import styled from 'styled-components'

import ArrowDownIcon from '../../assets/svg-icons/arrow-down-white-icon.svg'
import ArrowUpIcon from '../../assets/svg-icons/arrow-up-white-icon.svg'

export interface StyleProps {
  change: number
}

export const StyledWrapper = styled.span<StyleProps>`
  display: flex;
  align-items: center;
  padding: 4px 9px;
  border-radius: 8px;
  background-color: ${p => p.change > 0 ? '#2AC194' : '#F75A3A'};
  width: 62px;
  height: 24px;
`
export const PriceChange = styled.span<StyleProps>`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 11px;
  font-weight: 400;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.background01};
`

export const ArrowBase = styled.span`
  width: 12px;
  height: 11px;
  background-repeat: no-repeat;
  background-size: contain;
  margin-right: 2px;
  background-position: center center;
`

export const ArrowUp = styled(ArrowBase)`
  background-image: url(${ArrowUpIcon});
`

export const ArrowDown = styled(ArrowBase)`
  background-image: url(${ArrowDownIcon});
`
