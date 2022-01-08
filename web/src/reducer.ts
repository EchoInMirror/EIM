/* eslint-disable @typescript-eslint/no-empty-function */
import { createContext } from 'react'

export enum ReducerTypes {
  ChangeActiveTrack
}

export const initialState = {
  activeTrack: ''
}

export interface Action { type: ReducerTypes, [key: string]: any }

export const reducer = (state: typeof initialState, action: Action): typeof initialState => {
  switch (action.type) {
    case ReducerTypes.ChangeActiveTrack:
      return {
        ...state,
        activeTrack: action.activeTrack
      }

    default:
      return state
  }
}

// eslint-disable-next-line @typescript-eslint/no-unused-vars
export const GlobalDataContext = createContext([initialState, (_action: Action) => { }] as const)
