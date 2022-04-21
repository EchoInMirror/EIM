import './Mixer.less'
import React, { useEffect, useState } from 'react'
import packets, { ClientboundPacket } from '../../packets'
import useGlobalData from '../reducer'
import Marquee from 'react-fast-marquee'
import { setType, setWidth } from './SideBar'
import { levelMarks } from '../utils'
import { Slider, IconButton, Card, Divider, Stack, getLuminance, Button, Tooltip, useTheme } from '@mui/material'

import Power from '@mui/icons-material/Power'
import VolumeUp from '@mui/icons-material/VolumeUp'
import VolumeOff from '@mui/icons-material/VolumeOff'
import LoadingButton from '@mui/lab/LoadingButton'

const TrackPlugin: React.FC<{ plugin: packets.TrackInfo.IPluginData, uuid: string, index: number }> = ({ plugin, uuid, index }) => {
  return (
    <>
      <Tooltip title='233' arrow placement='right'>
        <Button
          fullWidth
          size='small'
          color='inherit'
          onClick={() => $client.rpc.openPluginWindow({ uuid, index })}
        >
          <Marquee gradient={false} pauseOnHover>{plugin.name}&nbsp;&nbsp;&nbsp;&nbsp;</Marquee>
        </Button>
      </Tooltip>
      <Divider variant='middle' />
    </>
  )
}

const defaultPan = [0, 0]
const indexToUUID: Record<number, string> = { }
const leftLevels: Record<string, HTMLDivElement | null> = {}
const rightLevels: Record<string, HTMLDivElement | null> = {}
const levelTexts: Record<string, HTMLSpanElement | null> = {}

const Track: React.FC<{ info: packets.ITrackInfo, active: boolean }> = ({ info, active }) => {
  const [pan, setPan] = useState(defaultPan)
  const [loading, setLoading] = useState(false)
  const [state, dispatch] = useGlobalData()
  const theme = useTheme()

  const uuid = info.uuid!
  const isMaster = !uuid
  useEffect(() => {
    setPan(old => (old[0] === 0 ? old[1] : old[0]) === info.pan ? old : info.pan! > 0 ? [0, info.pan!] : [info.pan!, 0])
  }, [info.pan])

  const color = uuid ? info.color! : theme.palette.primary.main

  return (
    <Card className='track' sx={active ? { border: theme => 'solid 1px ' + theme.palette.primary.main } : undefined}>
      <Button
        fullWidth
        disableElevation
        className='name'
        variant='contained'
        color={isMaster ? 'primary' : undefined}
        style={isMaster ? undefined : { backgroundColor: color, color: getLuminance(color) > 0.5 ? '#000' : '#fff' }}
        onClick={() => $client.rpc.openPluginWindow({ uuid })}
        onDragOver={e => !loading && $dragObject?.type === 'loadPlugin' && $dragObject.isInstrument && e.preventDefault()}
        onDrop={() => {
          if (loading || !$dragObject?.isInstrument || $dragObject.type !== 'loadPlugin') return
          setLoading(true)
          $client.rpc.loadVST({ uuid, identifier: $dragObject.data }).finally(() => setLoading(false))
        }}
      >
        <Marquee gradient={false} pauseOnHover>{isMaster ? '主轨道' : info.name}&nbsp;&nbsp;&nbsp;&nbsp;</Marquee>{info.hasInstrument && <Power fontSize='small' />}
      </Button>
      <div className='content'>
        <Slider
          size='small'
          value={pan}
          min={-100}
          sx={{ [`& .MuiSlider-thumb[data-index="${pan[0] === 0 ? 0 : 1}"]`]: { opacity: 0 } }}
          onChange={(_, val) => {
            const [a, b] = val as number[]
            const cur = a === 0 ? b : a
            $client.rpc.updateTrackInfo({ uuid, pan: cur })
            setPan(cur > 0 ? [0, cur] : [cur, 0])
          }}
        />
        <div className='level'>
          <IconButton size='small' className='mute' onClick={() => $client.rpc.updateTrackInfo({ uuid, muted: !info.muted })}>
            {info.muted ? <VolumeOff fontSize='small' /> : <VolumeUp fontSize='small' />}
          </IconButton>
          <Slider
            size='small'
            orientation='vertical'
            valueLabelDisplay='auto'
            value={Math.sqrt(info.volume!) * 100}
            max={140}
            min={0}
            marks={levelMarks}
            classes={{ markLabel: 'volume-mark', root: 'volume-slider' }}
            valueLabelFormat={val => `${Math.round(val)}% ${val > 0 ? (Math.log10((val / 100) ** 2) * 20).toFixed(2) + '分贝' : '静音'}`}
            onChange={(_, val) => {
              const volume = info.volume = ((val as number) / 100) ** 2
              $client.rpc.updateTrackInfo({ uuid, volume })
              dispatch({ tracks: { ...state.tracks, [uuid]: info } })
            }}
          />
          <div className='left'><span ref={it => (levelTexts[uuid] = it)}>-12.0</span><div ref={it => (leftLevels[uuid] = it)} /></div>
          <div className='right'><div ref={it => (rightLevels[uuid] = it)} /></div>
        </div>
      </div>
      <Divider className='mid-divider' />
      <div className='plugins'>
        {info.plugins!.map((it, i) => <TrackPlugin key={i} plugin={it} uuid={uuid} index={i} />)}
        <LoadingButton
          size='small'
          loading={loading}
          sx={{ width: '100%', borderRadius: 0 }}
          onClick={() => {
            setType(1)
            setWidth(width => width || 200)
          }}
          onDragOver={e => !loading && $dragObject?.type === 'loadPlugin' && !$dragObject.isInstrument && e.preventDefault()}
          onDrop={() => {
            if (loading || $dragObject?.type !== 'loadPlugin' || $dragObject.isInstrument) return
            setLoading(true)
            $client.rpc.loadVST({ uuid, identifier: $dragObject.data }).finally(() => setLoading(false))
          }}
        >
          添加插件
        </LoadingButton>
      </div>
    </Card>
  )
}

const Mixer: React.FC = () => {
  const [globalData] = useGlobalData()
  const theme = useTheme()
  useEffect(() => {
    const handlePing = (data: packets.IClientboundPing) => {
      const levels = data.levels!
      const size = levels.length / 2 | 0
      for (let i = 0; i < size; i++) {
        const uuid = indexToUUID[i]
        if (uuid == null) continue
        const i2 = i * 2
        const left = leftLevels[uuid]
        const right = rightLevels[uuid]
        const span = levelTexts[uuid]
        const leftLevel = levels[i2]
        const rightLevel = levels[i2 + 1]
        const maxLevel = Math.log10(Math.max(leftLevel, rightLevel)) * 20
        const color = maxLevel < -6 ? theme.palette.primary.main : maxLevel < 0 ? theme.palette.warning.main : theme.palette.error.main
        if (left) {
          left.style.height = Math.min(Math.sqrt(leftLevel) / 1.4 * 100, 100) + '%'
          left.style.backgroundColor = color
        }
        if (right) {
          right.style.height = Math.min(Math.sqrt(rightLevel) / 1.4 * 100, 100) + '%'
          right.style.backgroundColor = color
        }
        if (span) span.innerText = maxLevel === -Infinity ? '-inf' : maxLevel.toFixed(2)
      }
    }
    $client.on(ClientboundPacket.Ping, handlePing)
    return () => { $client.on(ClientboundPacket.Ping, handlePing) }
  }, [theme])

  const tracks: JSX.Element[] = []
  let i = 0
  for (const uuid in globalData.tracks!) {
    indexToUUID[i++] = uuid
    tracks.push(<Track key={uuid} info={globalData.tracks[uuid]!} active={globalData.activeTrack === uuid} />)
  }

  return (
    <Stack className='mixer' spacing={1} direction='row'>
      {tracks}
    </Stack>
  )
}

export default Mixer
