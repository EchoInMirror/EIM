/* eslint-disable @typescript-eslint/no-empty-function */
import { createContext, useContext } from 'react'

export interface TrackInfo {
  uuid: string
  name: string
  muted: boolean
  solo: boolean
  hasInstrument: boolean
  color: string
  volume: number
}

export enum ReducerTypes {
  ChangeActiveTrack,
  SetProjectStatus,
  SetTrackMidiData,
  SetTrackInfo,
  SetMaxNoteTime,
  UpdateTrack
}

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
  tracks: [] as TrackInfo[]
}

export interface Action { type: ReducerTypes, [key: string]: any }

export const reducer = (state: typeof initialState, action: Action): typeof initialState => {
  const { type, ...others } = action
  switch (type) {
    case ReducerTypes.ChangeActiveTrack:
      return {
        ...state,
        activeTrack: action.activeTrack
      }
    case ReducerTypes.SetProjectStatus:
      return {
        ...state,
        ...others
      }
    case ReducerTypes.SetTrackMidiData:
      return {
        ...state,
        maxNoteTime: action.maxNoteTime,
        trackMidiData: action.trackMidiData
      }
    case ReducerTypes.SetMaxNoteTime:
      return state.maxNoteTime === action.maxNoteTime
        ? state
        : {
            ...state,
            maxNoteTime: action.maxNoteTime
          }
    case ReducerTypes.SetTrackInfo:
      return {
        ...state,
        tracks: action.tracks
      }
    case ReducerTypes.UpdateTrack: {
      const tracks = state.tracks.slice()
      // eslint-disable-next-line @typescript-eslint/no-unused-vars
      const { type, index, ...others } = action
      tracks[index] = { ...tracks[index], ...others }
      return { ...state, tracks }
    }
    default:
      return state
  }
}

export type DispatchType = (action: Action) => void

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const GlobalDataContext = createContext([initialState, (_action: Action) => { }] as const)

export default () => useContext(GlobalDataContext)
