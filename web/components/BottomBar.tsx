import './BottomBar.less'
import React, { useState, useEffect, useContext, memo } from 'react'
import { Resizable } from 're-resizable'
import { Paper, Box, Tooltip } from '@mui/material'
import { BottomBarContext } from '../reducer'
import packets, { ClientboundPacket } from '../../packets'
import { TimeAgoNoSuffix } from './TimeAgo'
import prettyBytes from 'pretty-bytes'
import LinkIcon from '@mui/icons-material/Link'
import LinkOffIcon from '@mui/icons-material/LinkOff'
import MusicNote from '@mui/icons-material/MusicNote'
import MemoryOutlined from '@mui/icons-material/MemoryOutlined'
import Refresh from '@mui/icons-material/Refresh'
import WarningAmber from '@mui/icons-material/WarningAmber'
import EventNote from '@mui/icons-material/EventNote'
import FolderOpen from '@mui/icons-material/FolderOpen'
import SourceBranch from 'mdi-material-ui/SourceBranch'
import Piano from 'mdi-material-ui/Piano'
import Tune from 'mdi-material-ui/Tune'
import SettingsInputSvideo from '@mui/icons-material/SettingsInputSvideo'

import Mixer from './Mixer'
import Editor from './Editor'

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
      sx={{
        backgroundColor: theme => connected ? theme.palette.primary.main : theme.palette.error.main,
        color: theme => theme.palette.primary.contrastText,
        padding: '0 4px',
        marginRight: 0
      }}
    >
      {connected ? <LinkIcon fontSize='small' /> : <LinkOffIcon fontSize='small' />}
    </Box>
  )
})

const defaultSystemInfo = { cpu: 0, memory: '0 MB', time: 0, events: 0 }
const SystemStatus = memo(function SystemStatus () {
  const [info, setInfo] = useState<packets.IClientboundPong & { time: number }>(defaultSystemInfo as any)
  useEffect(() => {
    const fn = () => {
      if (!$client.isConnected) return
      const time = Date.now()
      $client.rpc.ping({ }).then((data: any) => {
        data.time = Date.now() - time
        data.memory = prettyBytes(data.memory)
        setInfo(data)
      })
    }
    const timer = setInterval(fn, 1000)
    fn()
    return () => clearInterval(timer)
  }, [])
  return (
    <span className='auto-space'>
      <MusicNote fontSize='small' />
      <span style={{ margin: '0 6px 0 0' }}>{info.events}</span>
      <MemoryOutlined fontSize='small' />
      <span>{info.cpu}%</span>
      <span>{info.memory}</span>
      <span>{info.time}ms</span>
    </span>
  )
})

const ProjectStatus = memo(function ProjectStatus () {
  const [time, setTime] = useState(0)
  const [root, setRoot] = useState('')
  useEffect(() => {
    const fn = (data: packets.IProjectStatus) => {
      if (data.projectTime != null) setTime(Date.now() - data.projectTime * 1000)
      if (data.projectRoot != null) setRoot(data.projectRoot)
    }
    $client.on(ClientboundPacket.SetProjectStatus, fn)
    return () => { $client.off(ClientboundPacket.SetProjectStatus, fn) }
  }, [])
  let index = root.lastIndexOf('/')
  if (!~index) index = root.lastIndexOf('\\')
  let rootName = root.slice(index + 1)
  if (/\d{13}/.test(rootName)) rootName = '临时项目'
  return (
    <span className='auto-space'>
      <FolderOpen className='smaller' />
      <Tooltip arrow title={root} placement='top'><span>{rootName}</span></Tooltip>
      <EventNote className='smaller' />
      <TimeAgoNoSuffix live date={time} />
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
        <ProjectStatus />
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

export const actions: Record<string, { title: string, icon: JSX.Element, component: JSX.Element }> = {
  'eim:editor': {
    title: '钢琴窗',
    icon: <Piano />,
    component: <Editor />
  },
  'eim:mixer': {
    title: '混音台',
    icon: <Tune sx={{ transform: 'rotate(90deg)' }} />,
    component: <Mixer />
  }
}

const BottomBar: React.FC = () => {
  const [height, setHeight] = useState(() => +localStorage.getItem('eim:bottomBar:height')! || 40)
  const action = actions[useContext(BottomBarContext)[0]]
  return (
    <>
      {action && (
        <Resizable
          onResizeStop={(_, __, ___, d) => {
            const h = height + d.height
            setHeight(h)
            localStorage.setItem('eim:bottomBar:height', h.toString())
          }}
          enable={{ top: true }}
          className='bottom-bar'
          minHeight={0}
          size={{ height: height } as any as { width: number, height: number }}
        >
          <Paper
            square
            id='bottom-bar'
            elevation={3}
            sx={{ borderTop: theme => '1px solid ' + theme.palette.primary.main, background: theme => theme.palette.background.bright, zIndex: 2 }}
          >
            {action.component}
          </Paper>
        </Resizable>
      )}
      <StatusBar />
    </>
  )
}

export default BottomBar
