import './BottomBar.less'
import React, { useState, useEffect, memo } from 'react'
import { Resizable } from 're-resizable'
import { Paper, Box } from '@mui/material'
import LinkIcon from '@mui/icons-material/Link'
import LinkOffIcon from '@mui/icons-material/LinkOff'
import MusicNote from '@mui/icons-material/MusicNote'
import MemoryOutlined from '@mui/icons-material/MemoryOutlined'
import Refresh from '@mui/icons-material/Refresh'
import WarningAmber from '@mui/icons-material/WarningAmber'
import SourceBranch from 'mdi-material-ui/SourceBranch'
import SettingsInputSvideo from '@mui/icons-material/SettingsInputSvideo'

import Mixer from './Mixer'
import Editor from './Editor'

// eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-empty-function
export let setIsMixer = (_val: boolean) => { }

const ConnectStatus = memo(function ConnectStatus () {
  const [connected, setConnected] = useState(true)
  useEffect(() => {
    const onClose = (msg: string) => {
      console.info(msg)
      setConnected(false)
    }
    $client.on('websocket:close', onClose)
    return () => { $client.off('websocket:close', onClose) }
  }, [])

  return (
    <Box
      component='span'
      sx={{ backgroundColor: theme => theme.palette.primary.main, color: theme => theme.palette.primary.contrastText, padding: '0 4px', marginRight: 0 }}
    >
      {connected ? <LinkIcon fontSize='small' /> : <LinkOffIcon fontSize='small' />}
    </Box>
  )
})

const SystemStatus = memo(function SystemStatus () {
  return (
    <span className='auto-space'>
      <MusicNote fontSize='small' />
      <span style={{ margin: '0 6px 0 0' }}>23</span>
      <MemoryOutlined fontSize='small' />
      <span>40%</span>
      <span>1.3G</span>
      <span>23ms</span>
    </span>
  )
})

const Git = memo(function Git () {
  return (
    <span className='auto-space'>
      <SourceBranch className='smaller' />
      <span>master</span>
      <Refresh className='smaller' />
    </span>
  )
})

const StatusBar = memo(function StatusBar () {
  return (
    <Paper square component='footer' elevation={3}>
      <span>
        <Git />
        <span className='auto-space'>
          <WarningAmber className='smaller' />
          <span>0</span>
        </span>
      </span>
      <span>
        <span className='auto-space'>
          <SettingsInputSvideo className='smaller' />
          <span>ASIO4ALL</span>
        </span>
        <SystemStatus />
        <ConnectStatus />
      </span>
    </Paper>
  )
})

const BottomBar: React.FC = () => {
  const [isMixer, fn] = useState(false)
  setIsMixer = fn
  return (
    <>
      <Resizable enable={{ top: true }} className='bottom-bar' maxHeight='80vh' minHeight={0}>
        <Paper
          square
          id='bottom-bar'
          elevation={3}
          sx={{ borderTop: theme => '1px solid ' + theme.palette.primary.main, background: theme => theme.palette.background.bright, zIndex: 2 }}
        >
          {isMixer ? <Mixer /> : <Editor />}
        </Paper>
      </Resizable>
      <StatusBar />
    </>
  )
}

export default BottomBar
