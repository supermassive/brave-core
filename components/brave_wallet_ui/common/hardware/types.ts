import { EthereumSignedTx } from 'trezor-connect/lib/typescript'
import { BraveWallet } from '../../constants/types'

export const FilecoinNetworkTypes = [
  BraveWallet.FILECOIN_MAINNET, BraveWallet.FILECOIN_TESTNET
] as const
export type FilecoinNetwork = typeof FilecoinNetworkTypes[number]

export type HardwareWalletResponseCodeType =
  | 'deviceNotConnected'
  | 'deviceBusy'
  | 'openEthereumApp'
  | 'transactionRejected'

export interface SignHardwareTransactionType {
  success: boolean
  error?: string
  deviceError?: HardwareWalletResponseCodeType
}
export interface SignatureVRS {
  v: number
  r: string
  s: string
}

export type HardwareOperationResult = {
  success: boolean
  error?: string
  code?: string | number
}

export type SignHardwareTransactionOperationResult = HardwareOperationResult & {
  payload?: EthereumSignedTx
}

export type SignHardwareMessageOperationResult = HardwareOperationResult & {
  payload?: string
}

export interface TrezorBridgeAccountsPayload {
  success: boolean
  accounts: BraveWallet.HardwareWalletAccount[]
  error?: string
}

export enum LedgerDerivationPaths {
  LedgerLive = 'ledger-live',
  Legacy = 'legacy'
}

export enum TrezorDerivationPaths {
  Default = 'trezor'
}

const DerivationSchemeTypes = [LedgerDerivationPaths.LedgerLive, LedgerDerivationPaths.Legacy, TrezorDerivationPaths.Default] as const
export type HardwareDerivationScheme = typeof DerivationSchemeTypes[number]

export type GetAccountsHardwareOperationResult = HardwareOperationResult & {
  payload?: BraveWallet.HardwareWalletAccount[]
}

// Did not create a string for these yet since it is
// likely these names will be returned from another service
// that will be localized.
export const FilecoinNetworkLocaleMapping = {
  [BraveWallet.FILECOIN_MAINNET]: 'Filecoin Mainnet',
  [BraveWallet.FILECOIN_TESTNET]: 'Filecoin Testnet'
}

export const FilecoinAddressProtocolTypes = [
  BraveWallet.FilecoinAddressProtocol.BLS, BraveWallet.FilecoinAddressProtocol.SECP256K1
] as const

// These Protocols do not need to be localized but are used
// as a value in a locale string.
export const FilecoinAddressProtocolLocaleMapping = {
  [BraveWallet.FilecoinAddressProtocol.BLS]: 'BLS12-381',
  [BraveWallet.FilecoinAddressProtocol.SECP256K1]: 'secp256k1'
}
