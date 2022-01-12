import './Tracks.less'
import ByteBuffer from 'bytebuffer'
import LoadingButton from '@mui/lab/LoadingButton'
import VolumeUp from '@mui/icons-material/VolumeUp'
import PlayRuler from './PlayRuler'
import React, { useState, useEffect, createRef, useRef } from 'react'
import useGlobalData, { ReducerTypes, TrackMidiNoteData } from '../reducer'
import { Paper, Box, Toolbar, Button, Slider, Stack, IconButton, Divider, alpha, useTheme } from '@mui/material'
import { ClientboundPacket, TrackInfo } from '../Client'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const TrackActions: React.FC<{ info: TrackInfo }> = ({ info }) => {
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

const Track: React.FC<{ data: TrackMidiNoteData[], width: number }> = ({ data, width }) => {
  return (
    <div className='notes'>
      {data && data.map((it, i) => (
        <div
          key={i}
          style={{
            bottom: (it[0] / 132 * 100) + '%',
            left: it[2] * width,
            width: it[3] * width
          }}
        />
      ))}
    </div>
  )
}

const Grid: React.FC<{ timeSigNumerator: number, width: number }> = ({ width, timeSigNumerator }) => {
  const theme = useTheme()
  const rectsX = []
  for (let i = 1; i < timeSigNumerator; i++) rectsX.push(<rect width='1' height='3240' x={width * i * 4} y='0' key={i} />)
  return (
    <svg xmlns='http://www.w3.org/2000/svg' width='0' height='0' style={{ position: 'absolute' }}>
      <defs>
        <pattern id='playlist-grid' x='0' y='0' width={width * timeSigNumerator * 4} height='3240' patternUnits='userSpaceOnUse'>
          <rect width='1' height='3240' x='0' y='0' fill={alpha(theme.palette.divider, 0.44)} />
          <g fill={theme.palette.divider}>{rectsX}</g>
        </pattern>
      </defs>
    </svg>
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
  const [state] = useGlobalData()
  const [tracks, setTracks] = useState<TrackInfo[]>([])
  const [loading, setLoading] = useState(false)
  const [height] = useState(70)
  const [noteWidth] = useState(0.4)
  const playListRef = useRef<HTMLElement | null>(null)

  useEffect(() => {
    $client.refresh()
    $client.on(ClientboundPacket.SyncTrackInfo, buf => {
      let len = buf.readUint8()
      const arr: TrackInfo[] = []
      $client.trackNameToIndex = { }
      while (len-- > 0) {
        const it = readTrack(buf)
        $client.trackNameToIndex[it.uuid] = arr.push(it) - 1
      }
      $client.tracks = arr
      setTracks(arr)
    })
    return () => $client.off(ClientboundPacket.SyncTrackInfo)
  }, [])

  barLength = noteWidth * state.ppq
  const beatWidth = barLength / (16 / state.timeSigDenominator)

  return (
    <main className='tracks'>
      <Toolbar />
      <PlayRuler headRef={playHeadRef} noteWidth={noteWidth} movableRef={playListRef} />
      <Box className='wrapper' sx={{ backgroundColor: theme => theme.palette.background.default }}>
        <Paper square elevation={3} component='ol' sx={{ background: theme => theme.palette.background.bright, zIndex: 1, '& li': { height } }}>
          <Divider />
          {tracks.map(it => <React.Fragment key={it.uuid}><TrackActions info={it} /><Divider /></React.Fragment>)}
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
        <Box className='playlist' sx={{ '& .notes': { height }, '& .notes div': { height: height / 132 + 'px' } }} ref={playListRef}>
          <div style={{ width: (state.maxNoteTime + state.ppq * 4) * noteWidth }}>
            <Grid timeSigNumerator={state.timeSigNumerator} width={beatWidth} />
            <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
              <rect fill='url(#playlist-grid)' x='0' y='0' width='100%' height='100%' />
            </svg>
            {tracks.map(it => (
              <div key={it.uuid} style={{ backgroundColor: alpha(it.color, 0.1) }}>
                <Track data={state.trackMidiData[it.uuid]?.notes} width={noteWidth} />
                <Divider />
              </div>
            ))}
          </div>
        </Box>
      </Box>
    </main>
  )
}

export default Tracks
