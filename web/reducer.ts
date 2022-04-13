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
  position: 0,
  maxNoteTime: 0,
  startTime: 0,
  trackMidiData: { } as TrackMidiData,
  tracks: { } as Record<string, packets.ITrackInfo | undefined>
}

export type StateType = typeof initialState

export const reducer = (state: StateType, action: Partial<StateType>): StateType => ({ ...state, ...action })

export type DispatchType = (action: Partial<StateType>) => void

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const GlobalDataContext = createContext([initialState, (_action: Partial<StateType>) => { }] as const)

export default () => useContext(GlobalDataContext)
