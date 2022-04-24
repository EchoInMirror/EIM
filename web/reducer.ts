/* eslint-disable @typescript-eslint/no-empty-function */
import { createContext, useContext } from 'react'
import packets from '../packets'

export const initialState = {
  activeTrack: '',
  ppq: 96,
  bpm: 120,
  isPlaying: false,
  timeSigNumerator: 4,
  timeSigDenominator: 4,
  maxNoteTime: 0,
  tracks: { } as Record<string, packets.ITrackInfo | undefined>,
  remiUrl: 'http://abyss.apisium.cn:8010/remi_gen'
}

export type StateType = typeof initialState

export type DispatchType = (action: Partial<StateType>) => void

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const GlobalDataContext = createContext([initialState, (_action: Partial<StateType>) => { }] as const)

export default () => useContext(GlobalDataContext)

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const BottomBarContext = createContext(['' as string, (_action: string) => { }] as const)
