import '@fontsource/roboto/300.css'
import '@fontsource/roboto/400.css'
import '@fontsource/roboto/500.css'
import '@fontsource/roboto/700.css'
import './App.less'

import React, { useEffect, useReducer } from 'react'
import { CssBaseline, Paper } from '@mui/material'
import { createTheme, ThemeProvider } from '@mui/material/styles'
import { SnackbarProvider } from 'notistack'
import type { PaletteOptions } from '@mui/material/styles/createPalette'
import { zhCN } from '@mui/material/locale'

import AppBar from './AppBar'
import SideBar from './SideBar'
import BottomBar from './BottomBar'
import Tracks from './Tracks'
import { GlobalDataContext, reducer, initialState, StateType } from '../reducer'
import packets, { ClientboundPacket } from '../../packets'

const palette: PaletteOptions = {
  background: {
    default: 'hsl(210deg 79% 98%)',
    paper: 'hsl(210deg 79% 98%)',
    bright: 'hsl(210deg 79% 96%)',
    brighter: 'hsl(210deg 79% 92%)',
    brightest: 'hsl(210deg 79% 70%)',
    keyboardBlackKey: '#21222c',
    keyboardWhiteKey: '#eee'
  }
  // mode: 'dark',
  // primary: { main: '#8be9fd' },
  // secondary: { main: '#bd93f9' },
  // success: { main: '#50fa7b' },
  // error: { main: '#ff5555' },
  // background: {
  //   default: '#21222c',
  //   paper: '#21222c',
  //   bright: '#282a36',
  //   brighter: '#343746',
  //   brightest: '#6272a4',
  //   keyboardBlackKey: '#21222c',
  //   keyboardWhiteKey: '#eee'
  // }
}

const App: React.FC = () => {
  const theme = createTheme({ palette }, zhCN)

  useEffect(() => {
    document.documentElement.style.setProperty('--primary-color', theme.palette.primary.main)
    document.documentElement.style.setProperty('--paper-background-color', theme.palette.background.bright)
    document.documentElement.style.setProperty('--scrollbar-color', theme.palette.mode === 'dark' ? theme.palette.text.primary : theme.palette.primary.main)
    document.documentElement.style.setProperty('--keyboard-black-key-color', theme.palette.background!.keyboardBlackKey)
    document.documentElement.style.setProperty('--keyboard-white-key-color', theme.palette.background!.keyboardWhiteKey)
  }, [
    theme.palette.primary.main,
    theme.palette.background.bright,
    theme.palette.mode,
    theme.palette.text.primary,
    theme.palette.background!.paper,
    theme.palette.background!.keyboardBlackKey,
    theme.palette.background!.keyboardWhiteKey
  ])

  const ctx = useReducer(reducer, initialState) as any
  window.$globalData = ctx[0]
  window.$dispatch = ctx[1]

  useEffect(() => {
    const fn = (data: packets.IProjectStatus) => {
      if (typeof data.position === 'number' || data.isPlaying) {
        $dispatch({ ...(data as any), startTime: Date.now() })
      } else $dispatch(data as any)
    }
    $client.on(ClientboundPacket.SetProjectStatus, fn)
    $client.rpc.refresh({ })
    return () => { $client.off(ClientboundPacket.SetProjectStatus, fn) }
  }, [])

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <SnackbarProvider maxSnack={6}>
        <GlobalDataContext.Provider value={ctx}>
          <AppBar />
          <SideBar />
          <Paper square elevation={3} className='main-content' sx={{ background: theme => theme.palette.background.bright }}>
            <Tracks />
            <BottomBar />
          </Paper>
        </GlobalDataContext.Provider>
      </SnackbarProvider>
    </ThemeProvider>
  )
}

export default App
