import './Tracks.less'
import ByteBuffer from 'bytebuffer'
import React, { useRef, useState, useEffect } from 'react'
import LoadingButton from '@mui/lab/LoadingButton'
import { Paper, Box, Toolbar, Button, useTheme, Slider, Stack, IconButton } from '@mui/material'
import { VolumeUp } from '@mui/icons-material'
import { ClientboundPacket } from '../Client'

interface TrackInfo {
  uuid: string
  name: string
  muted: boolean
  solo: boolean
  color: string
  volume: number
}

const Track: React.FC<{ info: TrackInfo }> = ({ info }) => {
  return (
    <li>
      <div className='color' style={{ backgroundColor: info.color }} />
      <div className='title'>
        <Button className='solo' variant='outlined' />
        <span className='name'>{info.name}</span>
      </div>
      <div>
        <Stack spacing={1} direction='row' alignItems='center'>
          <IconButton size='small'><VolumeUp fontSize='small' /></IconButton><Slider size='small' valueLabelDisplay='auto' value={info.volume} />
        </Stack>
      </div>
    </li>
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
  const theme = useTheme()
  const ref = useRef<HTMLDivElement | null>(null)
  const [tracks, setTracks] = useState<TrackInfo[]>([])
  const [loading, setLoading] = useState(false)

  useEffect(() => {
    if (!ref.current) return
    const fn = () => {
      const elm = document.getElementById('bottom-bar')
      if (!elm) return
      if (ref.current) ref.current.style.height = (window.innerHeight - ref.current.offsetTop - elm.clientHeight) + 'px'
    }
    fn()
    document.addEventListener('resize', fn)
    return () => document.removeEventListener('resize', fn)
  }, [ref.current])

  useEffect(() => {
    $client.refresh()
    $client.on(ClientboundPacket.SyncTrackInfo, buf => {
      let len = buf.readUint8()
      const arr: TrackInfo[] = []
      while (len-- > 0) arr.push(readTrack(buf))
      console.log(arr)
      setTracks(arr)
    })
    return () => $client.off(ClientboundPacket.SyncTrackInfo)
  }, [])

  return (
    <main className='tracks'>
      <Toolbar />
      <div ref={ref} className='wrapper'>
        <Paper square elevation={3} component='ol' sx={{ background: 'inherit', zIndex: 1 }}>
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
        <Box className='playlist' sx={{ backgroundColor: theme.palette.background.default }}>Play list</Box>
      </div>
    </main>
  )
}

export default Tracks
