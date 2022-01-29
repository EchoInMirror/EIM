import './Mixer.less'
import React, { useState } from 'react'
import useGlobalData, { TrackInfo, DispatchType, ReducerTypes } from '../reducer'
import { Slider, IconButton, Card, Divider, Stack, useTheme, getLuminance, Button } from '@mui/material'
import Marquee from 'react-fast-marquee'

import Power from '@mui/icons-material/Power'
import VolumeUp from '@mui/icons-material/VolumeUp'
import VolumeOff from '@mui/icons-material/VolumeOff'
import LoadingButton from '@mui/lab/LoadingButton'

const defaultPan = [50, 50]

const Track: React.FC<{ info: TrackInfo, active: boolean, index: number, dispatch: DispatchType }> = ({ info, active, index, dispatch }) => {
  const [pan, setPan] = useState(defaultPan)
  const [loading] = useState(false)

  return (
    <Card className='track' sx={active ? { border: theme => 'solid 1px ' + theme.palette.primary.main } : undefined}>
      <Button
        fullWidth
        disableElevation
        className='name'
        variant='contained'
        style={{ backgroundColor: info.color, color: getLuminance(info.color) > 0.5 ? '#000' : '#fff' }}
        onDragOver={e => $dragObject?.type === 'loadPlugin' && $dragObject.isInstrument && e.preventDefault()}
        onDrop={() => {
          if (!$dragObject?.isInstrument || $dragObject.type !== 'loadPlugin') return
          console.log($dragObject)
          // setLoading(true)
          // $client.createTrack(state.tracks.length, data.data).finally(() => setLoading(false))
        }}
      >
        {info.hasInstrument && <Power fontSize='small' />}<Marquee gradient={false} pauseOnHover>{info.name}</Marquee>
      </Button>
      <div className='content'>
        <Slider
          size='small'
          value={pan}
          sx={{ [`& .MuiSlider-thumb[data-index="${pan[0] === 50 ? 0 : 1}"]`]: { opacity: 0 } }}
          onChange={(_, val) => {
            const [a, b] = val as number[]
            const cur = a === 50 ? b : a
            setPan(cur > 50 ? [50, cur] : [cur, 50])
          }}
        />
        <div className='level'>
          <IconButton size='small' className='mute' onClick={() => $client.updateTrackInfo(index, undefined, undefined, -1, !info.muted, info.solo)}>
            {info.muted ? <VolumeOff fontSize='small' /> : <VolumeUp fontSize='small' />}
          </IconButton>
          <Slider
            size='small'
            orientation='vertical'
            valueLabelDisplay='auto'
            value={Math.sqrt(info.volume) * 100}
            max={140}
            min={0}
            valueLabelFormat={val => `${Math.round(val)}% ${val > 0 ? (Math.log10((val / 100) ** 2) * 20).toFixed(2) + '分贝' : '静音'}`}
            onChange={(_, val) => {
              const volume = ((val as number) / 100) ** 2
              $client.updateTrackInfo(index, undefined, undefined, volume, info.muted, info.solo)
              dispatch({ type: ReducerTypes.UpdateTrack, index, volume })
            }}
          />
          <div className='left'><span>-12.0</span><div /></div>
          <div className='right'><div /></div>
        </div>
      </div>
      <Divider className='mid-divider' />
      <div className='plugins'>
        <Marquee gradient={false} pauseOnHover>FabFilter Pro-Q 3</Marquee>
        <Divider variant='middle' />
        <Marquee gradient={false} pauseOnHover>FabFilter Pro-Q 3</Marquee>
        <Divider variant='middle' />
        <LoadingButton
          size='small'
          loading={loading}
          sx={{ width: '100%', borderRadius: 0 }}
          onDragOver={e => $dragObject?.type === 'loadPlugin' && e.preventDefault()}
        >
          添加插件
        </LoadingButton>
      </div>
    </Card>
  )
}

const Mixer: React.FC = () => {
  const masterTrack: TrackInfo = {
    uuid: '0000-0000-0000-0000-000000000000',
    name: '主轨道',
    muted: false,
    solo: false,
    hasInstrument: false,
    color: useTheme().palette.primary.main,
    volume: 1
  }
  const [globalData, dispatch] = useGlobalData()
  return (
    <Stack className='mixer' spacing={1} direction='row'>
      <Track key={masterTrack.uuid} info={masterTrack} active={false} index={-1} dispatch={dispatch} />
      {globalData.tracks.map((it, i) => <Track key={it.uuid} info={it} active={globalData.activeTrack === it.uuid} index={i} dispatch={dispatch} />)}
    </Stack>
  )
}

export default Mixer
