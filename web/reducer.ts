/* eslint-disable @typescript-eslint/no-empty-function */
import { createContext, useContext } from 'react'
import packets from '../packets'

export type TrackMidiNoteData = [number, number, number, number]
export type TrackMidiData = Record<string, { notes: TrackMidiNoteData[] }>

export const initialState = {
  activeTrack: '',
  ppq: 96,
  bpm: 120,
  isPlaying: false,
  timeSigNumerator: 4,
  timeSigDenominator: 4,
  currentTime: 0,
  maxNoteTime: 0,
  trackMidiData: { } as TrackMidiData,
  tracks: { } as Record<string, packets.ITrackInfo | undefined>
}

export const reducer = (state: typeof initialState, action: Partial<typeof initialState>): typeof initialState => ({ ...state, ...action })

export type DispatchType = (action: Partial<typeof initialState>) => void

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const GlobalDataContext = createContext([initialState, (_action: Partial<typeof initialState>) => { }] as const)

export default () => useContext(GlobalDataContext)
