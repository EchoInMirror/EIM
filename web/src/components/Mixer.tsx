import './Mixer.less'
import React, { useState } from 'react'
import useGlobalData, { TrackInfo } from '../reducer'
import { Slider, IconButton, Card, Divider, Stack, useTheme } from '@mui/material'
import Marquee from 'react-fast-marquee'

import Power from '@mui/icons-material/Power'
import VolumeUp from '@mui/icons-material/VolumeUp'
import VolumeMute from '@mui/icons-material/VolumeMute'
import LoadingButton from '@mui/lab/LoadingButton'

const defaultPan = [50, 50]

const Track: React.FC<{ info: TrackInfo, active: boolean }> = ({ info, active }) => {
  const [pan, setPan] = useState(defaultPan)
  return (
    <Card className='track' sx={active ? { border: theme => 'solid 1px ' + theme.palette.primary.main } : undefined}>
      <div className='name' style={{ backgroundColor: info.color }}><Power fontSize='small' /> {info.name}</div>
      <div className='content'>
        <Slider
          size='small'
          value={pan}
          onChange={(_, val) => {
            const [a, b] = val as number[]
            const cur = a === 50 ? b : a
            setPan(cur > 50 ? [50, cur] : [cur, 50])
          }}
        />
        <div className='level'>
          <IconButton size='small' className='mute'>
            {info.muted ? <VolumeMute fontSize='small' /> : <VolumeUp fontSize='small' />}
          </IconButton>
          <Slider
            size='small'
            orientation='vertical'
            defaultValue={30}
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
        <LoadingButton sx={{ width: '100%', borderRadius: 0 }} size='small'>
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
    color: useTheme().palette.primary.main,
    volume: 1
  }
  const [globalData] = useGlobalData()
  return (
    <Stack className='mixer' spacing={1} direction='row'>
      <Track key={masterTrack.uuid} info={masterTrack} active={false} />
      {globalData.tracks.map(it => <Track key={it.uuid} info={it} active={globalData.activeTrack === it.uuid} />)}
    </Stack>
  )
}

export default Mixer
