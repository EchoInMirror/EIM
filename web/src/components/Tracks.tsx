import './Tracks.less'
import LoadingButton from '@mui/lab/LoadingButton'
import VolumeUp from '@mui/icons-material/VolumeUp'
import VolumeMute from '@mui/icons-material/VolumeMute'
import PlayRuler from './PlayRuler'
import React, { useState, useEffect, createRef, useRef } from 'react'
import useGlobalData, { ReducerTypes, TrackMidiNoteData, TrackInfo } from '../reducer'
import { ColorPicker } from 'material-ui-color'
import { colorMap } from '../utils'
import { Paper, Box, Toolbar, Button, Slider, Stack, IconButton, Divider, alpha, useTheme } from '@mui/material'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const TrackActions: React.FC<{ info: TrackInfo, index: number }> = ({ info, index }) => {
  const [state, dispatch] = useGlobalData()
  const [volume, setVolume] = useState(100)
  const [color, setColor] = useState(info.color)

  useEffect(() => setVolume(Math.sqrt(info.volume) * 100), [info.volume])
  useEffect(() => setColor(info.color), [info.color])
  return (
    <Box
      component='li'
      onClick={() => dispatch({ type: ReducerTypes.ChangeActiveTrack, activeTrack: info.uuid })}
      sx={state.activeTrack === info.uuid ? { backgroundColor: theme => theme.palette.background.brighter } : undefined}
    >
      <ColorPicker
        deferred
        disableAlpha
        hideTextfield
        value={color}
        palette={colorMap}
        onChange={color => $client.updateTrackInfo(index, undefined, '#' + color.hex, -1, info.muted, info.solo)}
      />
      <div className='title'>
        <Button className='solo' variant='outlined' />
        <span className='name'>{info.name}</span>
      </div>
      <div>
        <Stack spacing={1} direction='row' alignItems='center'>
          <IconButton size='small' sx={{ marginLeft: '-4px' }} onClick={() => $client.updateTrackInfo(index, undefined, undefined, -1, !info.muted, info.solo)}>
            {info.muted ? <VolumeMute fontSize='small' /> : <VolumeUp fontSize='small' />}
          </IconButton>
          <Slider
            size='small'
            valueLabelDisplay='auto'
            value={volume}
            sx={{ margin: '0!important' }}
            max={140}
            min={0}
            valueLabelFormat={val => `${Math.round(val)}% ${val > 0 ? (Math.log10((val / 100) ** 2) * 20).toFixed(2) + '分贝' : '静音'}`}
            onChange={(_, val) => {
              setVolume(val as number)
              $client.updateTrackInfo(index, undefined, undefined, ((val as number) / 100) ** 2, info.muted, info.solo)
            }}
          />
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

const noteWidths = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.75, 1, 1.5, 2, 3, 4.5]

const Tracks: React.FC = () => {
  const [state] = useGlobalData()
  const [loading, setLoading] = useState(false)
  const [height] = useState(70)
  const [noteWidth, setNoteWidth] = useState(0.4)
  const playListRef = useRef<HTMLElement | null>(null)

  useEffect(() => $client.refresh(), [])

  barLength = noteWidth * state.ppq
  const beatWidth = barLength / (16 / state.timeSigDenominator)

  return (
    <main className='tracks'>
      <Toolbar />
      <PlayRuler headRef={playHeadRef} noteWidth={noteWidth} movableRef={playListRef} />
      <div className='scale-slider'>
        <Toolbar />
        <Slider
          min={0}
          max={9}
          defaultValue={3}
          onChange={(_, val) => setNoteWidth(noteWidths[val as number])}
        />
      </div>
      <Box className='wrapper' sx={{ backgroundColor: theme => theme.palette.background.default }}>
        <Paper square elevation={3} component='ol' sx={{ background: theme => theme.palette.background.bright, zIndex: 1, '& li': { height } }}>
          <Divider />
          {state.tracks.map((it, i) => <React.Fragment key={it.uuid}><TrackActions info={it} index={i} /><Divider /></React.Fragment>)}
          <LoadingButton
            loading={loading}
            sx={{ width: '100%', borderRadius: 0 }}
            onClick={() => $client.createTrack(state.tracks.length, '', '轨道' + (state.tracks.length + 1))}
            onDragOver={e => e.preventDefault()}
            onDrop={e => {
              const str = e.dataTransfer.getData('application/json')
              if (!str) return
              const data = JSON.parse(str)
              if (!data.eim || data.type !== 'loadPlugin') return
              setLoading(true)
              $client.createTrack(state.tracks.length, data.data).finally(() => setLoading(false))
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
            {state.tracks.map(it => (
              <Box key={it.uuid} sx={{ backgroundColor: alpha(it.color, 0.1), '& .notes div': { backgroundColor: it.color } }}>
                <Track data={state.trackMidiData[it.uuid]?.notes} width={noteWidth} />
                <Divider />
              </Box>
            ))}
          </div>
        </Box>
      </Box>
    </main>
  )
}

export default Tracks
