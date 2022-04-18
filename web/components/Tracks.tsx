import './Tracks.less'
import LoadingButton from '@mui/lab/LoadingButton'
import PlayRuler from './PlayRuler'
import React, { useState, useEffect, createRef, useRef, useMemo, memo } from 'react'
import useGlobalData from '../reducer'
import packets from '../../packets'
import { ColorPicker } from 'mui-color'
import { colorMap, colorValues } from '../utils'
import { Paper, Box, Toolbar, Button, Slider, Stack, IconButton, Divider, alpha, useTheme } from '@mui/material'

import Power from '@mui/icons-material/Power'
import VolumeUp from '@mui/icons-material/VolumeUp'
import VolumeOff from '@mui/icons-material/VolumeOff'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const TrackActions: React.FC<{ info: packets.ITrackInfo }> = ({ info }) => {
  const [state, dispatch] = useGlobalData()
  const [color, setColor] = useState(info.color!)

  const uuid = info.uuid!

  useEffect(() => setColor(info.color!), [info.color])
  return (
    <Box
      component='li'
      onClick={() => dispatch({ activeTrack: uuid })}
      sx={state.activeTrack === uuid ? { backgroundColor: theme => theme.palette.background.brighter } : undefined}
    >
      <ColorPicker
        deferred
        disableAlpha
        hideTextfield
        value={color}
        palette={colorMap}
        onChange={(color: any) => $client.rpc.updateTrackInfo({ uuid, color: '#' + color.hex })}
      />
      <div className='title'>
        <Button className='solo' variant='outlined' />
        {info.hasInstrument && (
          <IconButton
            size='small'
            className='instrument'
            onClick={() => {
              console.log(Date.now())
              $client.rpc.openPluginWindow({ uuid })
            }}
          >
            <Power fontSize='small' />
          </IconButton>
        )}
        <span className='name'>{info.name}</span>
      </div>
      <div>
        <Stack spacing={1} direction='row' alignItems='center'>
          <IconButton size='small' sx={{ marginLeft: '-4px' }} onClick={() => $client.rpc.updateTrackInfo({ uuid, muted: !info.muted })}>
            {info.muted ? <VolumeOff fontSize='small' /> : <VolumeUp fontSize='small' />}
          </IconButton>
          <Slider
            size='small'
            valueLabelDisplay='auto'
            value={Math.sqrt(info.volume!) * 100}
            sx={{ margin: '0!important' }}
            max={140}
            min={0}
            valueLabelFormat={val => `${Math.round(val)}% ${val > 0 ? (Math.log10((val / 100) ** 2) * 20).toFixed(2) + '分贝' : '静音'}`}
            onChange={(_, val) => {
              const volume = info.volume = ((val as number) / 100) ** 2
              $client.rpc.updateTrackInfo({ uuid, volume })
              dispatch({ tracks: { ...state.tracks, [uuid]: info } })
            }}
          />
        </Stack>
      </div>
    </Box>
  )
}

const TrackNotes: React.FC<{ data: packets.IMidiMessage[], width: number }> = ({ data, width }) => {
  return (
    <div className='notes'>
      {useMemo(() => {
        const arr: JSX.Element[] = []
        const midiOn: Record<number, number | undefined> = { }
        data.forEach(({ time, data }, i) => {
          switch (data! & 0xf0) {
            case 0x90: // NoteOn
              midiOn[data! >> 8 & 0xff] = time || 0
              break
            case 0x80: { // NoteOff
              const key = data! >> 8 & 0xff
              const val = midiOn[key]
              if (val !== undefined) {
                midiOn[key] = undefined
                arr.push((
                  <div
                    key={i}
                    style={{
                      bottom: (key / 132 * 100) + '%',
                      left: val * width,
                      width: (time! - val) * width
                    }}
                  />
                ))
              }
            }
          }
        })
        return arr
      }, [data, width])}
    </div>
  )
}

const Grid = memo(function Grid ({ width, timeSigNumerator }: { timeSigNumerator: number, width: number }) {
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
})

const noteWidths = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.75, 1, 1.5, 2, 3, 4.5]

const Tracks: React.FC = () => {
  const [state] = useGlobalData()
  const [loading, setLoading] = useState(false)
  const [height] = useState(70)
  const [noteWidthLevel, setNoteWidthLevel] = useState(3)
  const playListRef = useRef<HTMLElement | null>(null)
  const noteWidth = noteWidths[noteWidthLevel]
  barLength = noteWidth * state.ppq
  const beatWidth = barLength / (16 / state.timeSigDenominator)

  const actions: JSX.Element[] = []
  const midis: JSX.Element[] = []
  let trackCount = 0
  for (const id in state.tracks) {
    if (!id) continue
    const it = state.tracks[id]!
    actions.push(<React.Fragment key={id}><TrackActions info={it} /><Divider variant='middle' /></React.Fragment>)
    midis.push((
      <Box key={id} sx={{ backgroundColor: alpha(it.color!, 0.1), '& .notes div': { backgroundColor: it.color! } }}>
        <div className='content'>
          <TrackNotes data={it.midi!} width={noteWidth} />
          {/* <div className='audio-clips'>
            {Array.from({ length: 32 }, (_, i) => (
              <div
                key={i}
                style={{
                  background: it.color!,
                  WebkitMaskBoxImage: 'url(http://127.0.0.1:8088/test.png)',
                  transform: `scaleX(${state.ppq / 96 / 6 * noteWidth})`,
                  left: barLength * i
                }}
              >
                <img src='http://127.0.0.1:8088/test.png' style={{ opacity: 0 }} />
              </div>
            ))}
          </div> */}
        </div>
        <Divider />
      </Box>
    ))
    trackCount++
  }

  return (
    <main className='tracks'>
      <Toolbar />
      <PlayRuler
        id='tracks'
        headRef={playHeadRef}
        noteWidth={noteWidth}
        movableRef={playListRef}
        onWidthLevelChange={v => setNoteWidthLevel(Math.max(Math.min(noteWidthLevel + (v ? -1 : 1), noteWidths.length - 1), 0))}
      />
      <div className='scale-slider'>
        <Toolbar />
        <Slider
          min={0}
          max={noteWidths.length - 1}
          value={noteWidthLevel}
          onChange={(_, val) => setNoteWidthLevel(val as number)}
        />
      </div>
      <Box className='wrapper' sx={{ backgroundColor: theme => theme.palette.background.default }}>
        <Paper square elevation={3} component='ol' sx={{ background: theme => theme.palette.background.bright, zIndex: 1, '& li': { height } }}>
          <Divider />{actions}
          <LoadingButton
            loading={loading}
            sx={{ width: '100%', borderRadius: 0 }}
            onClick={() => $client.rpc.createTrack({ name: '轨道' + (trackCount + 1), color: colorValues[colorValues.length * Math.random() | 0] })}
            onDragOver={e => $dragObject?.type === 'loadPlugin' && e.preventDefault()}
            onDrop={() => {
              if (!$dragObject || $dragObject.type !== 'loadPlugin') return
              setLoading(true)
              $client.rpc.createTrack({ name: '轨道' + (trackCount + 1), color: colorValues[colorValues.length * Math.random() | 0], identifier: $dragObject.data }).finally(() => setLoading(false))
            }}
          >
            新增轨道
          </LoadingButton>
        </Paper>
        <Box className='playlist' sx={{ '& .content': { height }, '& .notes div': { height: height / 132 + 'px' } }} ref={playListRef}>
          <div style={{ width: (state.maxNoteTime + state.ppq * 4) * noteWidth }}>
            <Grid timeSigNumerator={state.timeSigNumerator} width={beatWidth} />
            <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
              <rect fill='url(#playlist-grid)' x='0' y='0' width='100%' height='100%' />
            </svg>
            {midis}
          </div>
        </Box>
      </Box>
    </main>
  )
}

export default Tracks
