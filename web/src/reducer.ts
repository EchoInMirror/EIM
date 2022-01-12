/* eslint-disable @typescript-eslint/no-empty-function */
import { createContext, useContext } from 'react'

export enum ReducerTypes {
  ChangeActiveTrack,
  SetProjectStatus,
  SetTrackMidiData,
  SetMaxNoteTime
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
  startTime: Date.now(),
  currentTime: 0,
  maxNoteTime: 0,
  trackMidiData: { } as TrackMidiData
}

export interface Action { type: ReducerTypes, [key: string]: any }

export const reducer = (state: typeof initialState, action: Action): typeof initialState => {
  switch (action.type) {
    case ReducerTypes.ChangeActiveTrack:
      return {
        ...state,
        activeTrack: action.activeTrack
      }
    case ReducerTypes.SetProjectStatus:
      return {
        ...state,
        ppq: action.ppq,
        bpm: action.bpm,
        isPlaying: action.isPlaying,
        currentTime: action.currentTime,
        timeSigNumerator: action.timeSigNumerator,
        timeSigDenominator: action.timeSigDenominator,
        startTime: action.startTime
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
    default:
      return state
  }
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const GlobalDataContext = createContext([initialState, (_action: Action) => { }] as const)

export default () => useContext(GlobalDataContext)
