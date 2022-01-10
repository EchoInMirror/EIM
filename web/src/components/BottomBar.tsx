import './BottomBar.less'
import React, { useEffect, createRef, useRef, useState } from 'react'
import { Resizable } from 're-resizable'
import { PlayArrowRounded } from '@mui/icons-material'
import { Paper, Button, Box, useTheme, alpha } from '@mui/material'
import useGlobalData, { TrackMidiNoteData } from '../reducer'

export let barLength = 0
export const playHeadRef = createRef<HTMLSpanElement>()

const keyNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const keys: JSX.Element[] = []
for (let i = 0; i < 132; i++) {
  const name = keyNames[i % 12]
  let elm: string | JSX.Element = name + ' ' + (i / 12 | 0)
  if (!(i % 12)) elm = <b>{elm}</b>
  else if (name.length === 2) elm = <i>{elm}</i>
  keys.push(<Button key={i} data-eim-keyboard-key={i}>{elm}</Button>)
}

const scales = [true, false, true, false, true, true, false, true, false, true, false, true].reverse()

const EditorGrid: React.FC<{ width: number, height: number }> = ({ width, height }) => {
  const theme = useTheme()
  const rects = []
  const lines = []
  const highlightColor = alpha(theme.palette.primary.main, 0.1)
  for (let i = 0; i < 12; i++) { // <line x1='0' x2='80' y1={i * 18 + 0.5} y2={i * 18 + 0.5} key={i} />
    rects.push(<rect key={i} height={height} width='81' y={height * i} fill={scales[i] === (theme.palette.mode === 'dark') ? highlightColor : 'none'} />)
    lines.push(<line x1='0' x2='80' y1={(i + 1) * height} y2={(i + 1) * height} key={i} />)
  }
  return (
    <svg xmlns='http://www.w3.org/2000/svg' id='definition-svg' width='0' height='0' style={{ position: 'absolute' }}>
      <defs>
        <pattern id='editor-grid-x' x='0' y='0' width={width * 4} height='3240' patternUnits='userSpaceOnUse'>
          <rect width='1' height='3240' x='0' y='0' fill={alpha(theme.palette.divider, 0.34)} />
          <g fill={theme.palette.divider}>
            <rect width='1' height='3240' x={width} y='0' />
            <rect width='1' height='3240' x={width * 2} y='0' />
            <rect width='1' height='3240' x={width * 3} y='0' />
          </g>
        </pattern>
        <pattern id='editor-grid-y' x='0' y='0' width='80' height={12 * height} patternUnits='userSpaceOnUse'>
          {rects}
          <g strokeWidth='1' shapeRendering='crispEdges' stroke={theme.palette.divider}>
            {lines}
          </g>
        </pattern>
      </defs>
    </svg>
  )
}

const Notes: React.FC<{ data: TrackMidiNoteData[], width: number, ppq: number, height: number, timeSigNumerator: number, color: string }> =
  ({ data, width, height, ppq, timeSigNumerator, color }) => {
    console.log('render')
    const ref = useRef<HTMLDivElement | null>(null)
    useEffect(() => {
      const cur = ref.current
      if (!cur || !data) return
      cur.innerText = ''
      data.forEach(it => {
        const elm = document.createElement('div')
        elm.style.bottom = (it[0] * height) + 'px'
        elm.style.left = (it[2] / ppq * width * timeSigNumerator) + 'px'
        elm.style.width = (it[3] / ppq * width * timeSigNumerator) + 'px'
        elm.style.backgroundColor = alpha(color, 0.4 + 0.6 * it[1] / 127)
        cur.appendChild(elm)
      })
    })
    return (
      <div className='notes' key={width} ref={ref} />
    )
  }

const Editor: React.FC = () => {
  const [beatWidth] = useState(10)
  const [noteHeight] = useState(10)
  const [state] = useGlobalData()

  useEffect(() => {
    let pressedKeys: number[] = []
    const mousedown = (e: MouseEvent) => {
      const btn = e.target as HTMLButtonElement
      const key = btn?.dataset?.eimKeyboardKey
      if (!key) return
      const index = $client.trackNameToIndex[state.activeTrack]
      if (index == null) return
      const rect = btn.getBoundingClientRect()
      const keyId = +key & 127
      pressedKeys.push(keyId)
      $client.midiMessage(index, 0x90, keyId, Math.min(rect.width, Math.max(0, e.pageX - rect.left)) / rect.width * 127 | 0)
    }
    const mouseup = () => {
      if (!pressedKeys.length) return
      const index = $client.trackNameToIndex[state.activeTrack]
      if (index == null) return
      pressedKeys.forEach(it => $client.midiMessage(index, 0x80, it, 80))
      pressedKeys = []
    }
    document.addEventListener('mousedown', mousedown)
    document.addEventListener('mouseup', mouseup)
    return () => {
      document.removeEventListener('mousedown', mousedown)
      document.removeEventListener('mouseup', mouseup)
    }
  }, [state.activeTrack])

  barLength = beatWidth * state.timeSigNumerator

  return (
    <div className='editor'>
      <Box className='actions' sx={{ backgroundColor: theme => theme.palette.background.bright, width: 200, zIndex: 1 }}>
        当前轨道: {$client.tracks.find(it => it.uuid === state.activeTrack)?.name || '未选中'}
      </Box>
      <Paper square elevation={3} className='scrollable'>
        <Paper square elevation={3} className='ruler' sx={{ width: 1500 }}>
          <span className='play-head' ref={playHeadRef}>
            <PlayArrowRounded />
          </span>
        </Paper>
        <div>
          <Paper square elevation={6} className='keyboard' sx={{ '& button': { height: noteHeight } }}>
            {keys}
          </Paper>
          <Box
            className='notes-wrapper'
            style={{ transform: 'translateX(0px)' }}
            sx={{
              '& .notes div': {
                boxShadow: theme => theme.shadows[1],
                backgroundColor: theme => theme.palette.primary.main,
                height: noteHeight
              }
            }}
          >
            <EditorGrid width={beatWidth} height={noteHeight} />
            <svg xmlns='http://www.w3.org/2000/svg' width='2200' height='100%' style={{ position: 'absolute' }}>
              <rect fill='url(#editor-grid-y)' x='0' y='0' width='100%' height='100%' />
              <rect fill='url(#editor-grid-x)' x='0' y='0' width='100%' height='100%' />
            </svg>
            <Notes
              data={state.trackMidiData[state.activeTrack]?.notes}
              width={beatWidth}
              height={noteHeight}
              ppq={state.ppq}
              timeSigNumerator={state.timeSigNumerator}
              color={$client.tracks[$client.trackNameToIndex[state.activeTrack]]?.color || ''}
            />
          </Box>
        </div>
      </Paper>
    </div>
  )
}

const BottomBar: React.FC = () => {
  return (
    <Resizable enable={{ top: true }} className='bottom-bar' maxHeight='80vh'>
      <Paper
        square
        id='bottom-bar'
        elevation={3}
        sx={{ borderTop: theme => '1px solid ' + theme.palette.primary.main, background: theme => theme.palette.background.bright, zIndex: 2 }}
        component='footer'
      >
        <Editor />
      </Paper>
    </Resizable>
  )
}

export default BottomBar
