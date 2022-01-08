import '@fontsource/roboto/300.css'
import '@fontsource/roboto/400.css'
import '@fontsource/roboto/500.css'
import '@fontsource/roboto/700.css'
import './App.less'

import React, { useEffect, useReducer } from 'react'
import { CssBaseline, Box } from '@mui/material'
import { createTheme, ThemeProvider } from '@mui/material/styles'
import type { PaletteOptions } from '@mui/material/styles/createPalette'
import { zhCN } from '@mui/material/locale'

import AppBar from './AppBar'
import SideBar from './SideBar'
import BottomBar from './BottomBar'
import Tracks from './Tracks'
import { GlobalDataContext, reducer, initialState } from '../reducer'

const palette: PaletteOptions = {
  mode: 'dark',
  primary: { main: '#8be9fd' },
  secondary: { main: '#bd93f9' },
  success: { main: '#50fa7b' },
  error: { main: '#ff5555' },
  background: {
    default: '#21222c',
    paper: '#21222c',
    bright: '#282a36',
    brighter: '#343746',
    brightest: '#6272a4'
  }
}

const App: React.FC = () => {
  const theme = createTheme({ palette }, zhCN)
  useEffect(() => {
    document.documentElement.style.setProperty('--scrollbar-color', theme.palette.text.primary)
  }, [theme.palette.background!.paper])
  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <GlobalDataContext.Provider value={useReducer(reducer, initialState) as any}>
        <AppBar />
        <SideBar />
        <Box className='main-content' sx={{ background: theme => theme.palette.background.bright }}>
          <Tracks />
          <BottomBar />
        </Box>
      </GlobalDataContext.Provider>
    </ThemeProvider>
  )
}

export default App
