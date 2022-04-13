// eslint-disable-next-line @typescript-eslint/no-unused-vars
import { TypeBackground } from '@mui/material/styles/createPalette'
import { initialState, StateType } from './reducer'
import Client from './Client'

declare type EIMDragObject = Record<string | number, any> & { type: string }

/* eslint-disable no-unused-vars */
declare module '@mui/material/styles/createPalette' {
  interface TypeBackground {
    bright: string
    brighter: string
    brightest: string
    keyboardBlackKey: string
    keyboardWhiteKey: string
  }
}

declare global {
  const $client: Client
  const $globalData: StateType
  let $dragObject: EIMDragObject | undefined
  const $dispatch: (action: Partial<StateType>) => void

  interface Window {
    $client: Client
    $globalData: StateType
    $dragObject: EIMDragObject | undefined
    $dispatch: (action: Partial<StateType>) => void
  }
}
