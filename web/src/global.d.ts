// eslint-disable-next-line @typescript-eslint/no-unused-vars
import { TypeBackground } from '@mui/material/styles/createPalette'
import Client from './Client'

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

  interface Window {
    $client: Client
  }
}
