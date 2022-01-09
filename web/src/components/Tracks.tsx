import './Tracks.less'
import ByteBuffer from 'bytebuffer'
import React, { useState, useEffect } from 'react'
import LoadingButton from '@mui/lab/LoadingButton'
import { Paper, Box, Toolbar, Button, Slider, Stack, IconButton } from '@mui/material'
import { VolumeUp } from '@mui/icons-material'
import { ClientboundPacket, TrackInfo } from '../Client'
import useGlobalData, { ReducerTypes } from '../reducer'

const Track: React.FC<{ info: TrackInfo }> = ({ info }) => {
  const [state, dispatch] = useGlobalData()
  return (
    <Box
      component='li'
      onClick={() => dispatch({ type: ReducerTypes.ChangeActiveTrack, activeTrack: info.uuid })}
      sx={state.activeTrack === info.uuid ? { backgroundColor: theme => theme.palette.background.brighter } : undefined}
    >
      <div className='color' style={{ backgroundColor: info.color }} />
      <div className='title'>
        <Button className='solo' variant='outlined' />
        <span className='name'>{info.name}</span>
      </div>
      <div>
        <Stack spacing={1} direction='row' alignItems='center'>
          <IconButton size='small' sx={{ marginLeft: '-4px' }}>
            <VolumeUp fontSize='small' />
          </IconButton>
          <Slider size='small' valueLabelDisplay='auto' value={info.volume} sx={{ margin: '0!important' }} />
        </Stack>
      </div>
    </Box>
  )
}

const readTrack = (buf: ByteBuffer): TrackInfo => ({
  uuid: buf.readIString(),
  name: buf.readIString(),
  color: buf.readIString(),
  volume: buf.readUint8(),
  muted: !!buf.readUint8(),
  solo: !!buf.readUint8()
})

const Tracks: React.FC = () => {
  const [tracks, setTracks] = useState<TrackInfo[]>([])
  const [loading, setLoading] = useState(false)

  useEffect(() => {
    $client.refresh()
    $client.on(ClientboundPacket.SyncTrackInfo, buf => {
      let len = buf.readUint8()
      const arr: TrackInfo[] = []
      while (len-- > 0) arr.push(readTrack(buf))
      $client.tracks = arr
      setTracks(arr)
    })
    return () => $client.off(ClientboundPacket.SyncTrackInfo)
  }, [])

  return (
    <main className='tracks'>
      <Toolbar />
      <Box className='wrapper' sx={{ backgroundColor: theme => theme.palette.background.default }}>
        <Paper square elevation={3} component='ol' sx={{ background: theme => theme.palette.background.bright, zIndex: 1 }}>
          {tracks.map(it => <Track info={it} key={it.uuid} />)}
          <LoadingButton
            loading={loading}
            sx={{ width: '100%', borderRadius: 0 }}
            onClick={() => $client.createTrack(tracks.length, '')}
            onDragOver={e => e.preventDefault()}
            onDrop={e => {
              const str = e.dataTransfer.getData('application/json')
              if (!str) return
              const data = JSON.parse(str)
              if (!data.eim || data.type !== 'loadPlugin') return
              setLoading(true)
              $client.createTrack(tracks.length, data.data).finally(() => setLoading(false))
            }}
          >
            新增轨道
          </LoadingButton>
        </Paper>
        <Box className='playlist'>Play list</Box>
      </Box>
    </main>
  )
}

export default Tracks
