import './Tracks.less'
import React, { useState, useEffect, createRef, useRef, memo } from 'react'
import LoadingButton from '@mui/lab/LoadingButton'
import PlayRuler from './PlayRuler'
import ContentEditable from 'react-contenteditable'
import useGlobalData from '../reducer'
import packets from '../../packets'
import { ColorPicker } from 'mui-color'
import { colorMap, colorValues, levelMarks, tryLoadImage, FULL_BACKEND_PATH } from '../utils'
import {
  Paper, Box, Toolbar, Button, Slider, Stack, IconButton, Divider, alpha, useTheme,
  Menu, MenuItem, ListItemText, ListItemIcon, CircularProgress
} from '@mui/material'

import Power from '@mui/icons-material/Power'
import VolumeUp from '@mui/icons-material/VolumeUp'
import VolumeOff from '@mui/icons-material/VolumeOff'
import Delete from '@mui/icons-material/Delete'
import ArrowDownward from '@mui/icons-material/ArrowDownward'
import ArrowUpward from '@mui/icons-material/ArrowUpward'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const TrackActions = memo(function TrackActions ({ info, index }: { info: packets.ITrackInfo, index: number }) {
  const [state, dispatch] = useGlobalData()
  const [color, setColor] = useState(info.color!)
  const [name, setName] = useState(info.name!)
  const [contextMenu, setContextMenu] = React.useState<{ left: number, top: number } | undefined>()

  const uuid = info.uuid!

  useEffect(() => setColor(info.color!), [info.color])
  useEffect(() => setName(info.name!), [info.name])
  return (
    <Box
      component='li'
      onClick={() => dispatch({ activeTrack: uuid })}
      sx={state.activeTrack === uuid ? { backgroundColor: theme => theme.palette.background.brighter } : undefined}
      onContextMenu={(e: any) => {
        e.preventDefault()
        setContextMenu(contextMenu ? undefined : { left: e.clientX - 2, top: e.clientY - 4 })
      }}
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
            onClick={() => { $client.rpc.openPluginWindow({ uuid }) }}
          >
            <Power fontSize='small' />
          </IconButton>
        )}
        <ContentEditable
          className='name'
          tagName='span'
          html={name}
          spellCheck='false'
          onChange={e => setName(e.target.value)}
          onBlur={e => $client.rpc.updateTrackInfo({ uuid, name: e.target.innerText })}
          onKeyDown={e => {
            if (e.code !== 'Enter') return
            e.preventDefault()
            $client.rpc.updateTrackInfo({ uuid, name: (e.target as HTMLSpanElement).innerText })
          }}
        />
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
            marks={levelMarks}
            classes={{ markLabel: 'volume-mark' }}
            valueLabelFormat={val => `${Math.round(val)}% ${val > 0 ? (Math.log10((val / 100) ** 2) * 20).toFixed(2) + '分贝' : '静音'}`}
            onChange={(_, val) => {
              const volume = info.volume = ((val as number) / 100) ** 2
              $client.rpc.updateTrackInfo({ uuid, volume })
              dispatch({ tracks: { ...state.tracks, [uuid]: info } })
            }}
          />
        </Stack>
      </div>
      <Menu open={!!contextMenu} onClose={() => setContextMenu(undefined)} anchorReference='anchorPosition' anchorPosition={contextMenu}>
        <MenuItem
          onClick={() => {
            setContextMenu(undefined)
            $client.rpc.removeTrack({ value: uuid })
          }}
        >
          <ListItemIcon><Delete fontSize='small' /></ListItemIcon>
          <ListItemText>删除轨道</ListItemText>
        </MenuItem>
        <MenuItem
          disabled={!index}
          onClick={() => {
            setContextMenu(undefined)
          }}
        >
          <ListItemIcon><ArrowUpward fontSize='small' /></ListItemIcon>
          <ListItemText>上移</ListItemText>
        </MenuItem>
        <MenuItem
          onClick={() => {
            setContextMenu(undefined)
          }}
        >
          <ListItemIcon><ArrowDownward fontSize='small' /></ListItemIcon>
          <ListItemText>下移</ListItemText>
        </MenuItem>
      </Menu>
    </Box>
  )
})

const TrackNotes = memo(function TrackNotes ({ data, width }: { data: packets.IMidiMessage[], width: number }) {
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
  return (
    <div className='notes'>{arr}</div>
  )
})

const MaskBoxImage = memo(function MaskBoxImage ({ url, width }: { url: string, width: number }) {
  const [loaded, setLoaded] = useState(false)
  useEffect(() => {
    setLoaded(false)
    tryLoadImage(url).then(() => setLoaded(true))
  }, [url])
  return loaded ? <div style={{ WebkitMaskBoxImage: `url('${url}')`, width }} /> : <Box className='loading-sample'><CircularProgress size={20} /></Box>
})
const TrackSamples = memo(function TrackSamples ({ uuid, data, noteWidth, bpm, ppq }:
  { data: packets.ISampleData[], noteWidth: number, bpm: number, ppq: number, uuid: string }) {
  return (
    <div
      className='audio-clips'
      onMouseDown={e => {
        const target = e.target as HTMLDivElement
        if (e.button === 2 && target.dataset.sampleIndex != null) {
          e.preventDefault()
          $client.rpc.deleteSample({ uuid, index: [+target.dataset.sampleIndex!] })
        }
      }}
    >
      {data.map((it, i) => {
        return (
          <div
            key={i}
            data-sample-index={i}
            style={{
              width: (it.fullTime || it.duration! / 60 * bpm * ppq) * noteWidth,
              left: noteWidth * it.position!
            }}
          >
            <MaskBoxImage url={FULL_BACKEND_PATH + 'samples-preview/' + it.file + '.png'} width={(it.fullTime || it.duration! / 60 * bpm * ppq) * noteWidth} />
            <span className='sample-name'>{it.file}</span>
          </div>
        )
      })}
    </div>
  )
})

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
  const contents: JSX.Element[] = []
  let trackCount = 0
  for (const id in state.tracks) {
    if (!id) continue
    const it = state.tracks[id]!
    actions.push(<React.Fragment key={id}><TrackActions info={it} index={trackCount} /><Divider variant='middle' /></React.Fragment>)
    contents.push((
      <Box
        key={id}
        sx={{
          backgroundColor: alpha(it.color!, 0.1),
          '& .notes div': { backgroundColor: it.color! },
          '& .audio-clips > div': {
            backgroundColor: alpha(it.color!, 0.2),
            border: '1px solid ' + it.color!,
            '& .MuiCircularProgress-root': { color: it.color! },
            '& > div': {
              backgroundColor: it.color!
            }
          }
        }}
      >
        <div
          className='content'
          onDragOver={e => ($dragObject?.type === 'loadSample' || $dragObject?.type === 'loadMidi') && e.preventDefault()}
          onDrop={e => {
            if (!$dragObject) return
            switch ($dragObject.type) {
              case 'loadSample': {
                const alignment = +localStorage.getItem('eim:editor:alignment')! || state.ppq
                const position = Math.round((e.pageX - e.currentTarget.getBoundingClientRect().left) / noteWidth / alignment) * alignment
                $client.rpc.addSample({ uuid: id, data: [{ position, file: $dragObject.data }] })
                break
              }
              case 'loadMidi':
                $client.rpc.loadMidi({ uuid: id, file: $dragObject.data })
                break
            }
          }}
        >
          <TrackNotes data={it.midi!} width={noteWidth} />
          <TrackSamples data={it.samples!} noteWidth={noteWidth} ppq={state.ppq} bpm={state.bpm} uuid={id} />
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
              $client.rpc.createTrack({ name: '轨道' + (trackCount + 1), color: colorValues[colorValues.length * Math.random() | 0], identifier: $dragObject.data })
                .finally(() => setLoading(false))
            }}
          >
            新增轨道
          </LoadingButton>
        </Paper>
        <Box className='playlist' sx={{ '& .content': { height }, '& .notes div': { height: height / 132 + 'px' } }} ref={playListRef} onContextMenu={e => e.preventDefault()}>
          <div style={{ width: (state.maxNoteTime + state.ppq * 4) * noteWidth }}>
            <Grid timeSigNumerator={state.timeSigNumerator} width={beatWidth} />
            <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
              <rect fill='url(#playlist-grid)' x='0' y='0' width='100%' height='100%' />
            </svg>
            {contents}
          </div>
        </Box>
      </Box>
    </main>
  )
}

export default Tracks
