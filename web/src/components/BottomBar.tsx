import './BottomBar.less'
import React, { useEffect, useRef, useState, createRef } from 'react'
import useGlobalData, { TrackMidiNoteData } from '../reducer'
import PlayRuler from './PlayRuler'
import { Resizable } from 're-resizable'
import { Paper, Button, Box, useTheme, alpha, Slider } from '@mui/material'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const keyNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const keys: JSX.Element[] = []
for (let i = 0; i < 132; i++) {
  const name = keyNames[i % 12]
  let elm: string | JSX.Element = name + ' ' + (i / 12 | 0)
  if (!(i % 12)) elm = <b>{elm}</b>
  else if (name.length === 2) elm = <i>{elm}</i>
  keys.push(<Button key={i} data-eim-keyboard-key={i}>{elm}</Button>)
}

const noteWidths = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.75, 1, 1.5, 2, 3, 4.5]

const scales = [true, false, true, false, true, true, false, true, false, true, false, true].reverse()

const EditorGrid: React.FC<{ width: number, height: number, timeSigNumerator: number, timeSigDenominator: number }> = ({ width, height, timeSigNumerator, timeSigDenominator }) => {
  const theme = useTheme()
  const rects = []
  const lines = []
  const rectsX = []
  const highlightColor = alpha(theme.palette.primary.main, 0.1)
  const gridXDeepColor = alpha(theme.palette.divider, 0.26)
  const beats = 16 / timeSigDenominator
  for (let i = 0; i < 12; i++) {
    rects.push(<rect key={i} height={height} width='81' y={height * i} fill={scales[i] === (theme.palette.mode === 'dark') ? highlightColor : 'none'} />)
    lines.push(<line x1='0' x2='80' y1={(i + 1) * height} y2={(i + 1) * height} key={i} />)
  }
  for (let j = 0, cur = 0; j < timeSigNumerator; j++) {
    rectsX.push(<rect width='1' height='3240' x={width * cur++} y='0' fill={j ? gridXDeepColor : alpha(theme.palette.divider, 0.44)} key={j << 5} />)
    if (width >= 9.6) {
      for (let i = 1; i < beats; i++) {
        rectsX.push(<rect width='1' height='3240' x={width * cur++} y='0' key={j << 5 | i} />)
      }
    } else cur += beats - 1
  }
  return (
    <svg xmlns='http://www.w3.org/2000/svg' width='0' height='0' style={{ position: 'absolute' }}>
      <defs>
        <pattern id='editor-grid-x' x='0' y='0' width={width * beats * timeSigNumerator} height='3240' patternUnits='userSpaceOnUse'>
          <g fill={theme.palette.divider}>{rectsX}</g>
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

const scrollableRef = createRef<HTMLDivElement>()

let startX = 0
let startY = 0
let offsetY = 0
let offsetX = 0
let resizeDirection = 0
let grabbing = false
let selectedNotes: HTMLDivElement[] = []

const Notes: React.FC<{ data: TrackMidiNoteData[], width: number, height: number, ppq: number, color: string, alignment: number }> = ({ data, width, height, ppq, color, alignment }) => {
  const ref = useRef<HTMLDivElement | null>(null)
  const alignmentWidth = width * alignment

  console.log(ppq)

  useEffect(() => {
    const cur = ref.current
    if (!cur || !data) return
    cur.innerText = ''
    selectedNotes = []
    grabbing = false
    data.forEach(it => {
      const elm = document.createElement('div')
      elm.style.top = ((1 - it[0] / 132) * 100) + '%'
      elm.style.left = (it[2] * width) + 'px'
      if (it[2] === it[3]) {
        elm.style.width = '2px'
        elm.style.transform = 'translateX(-1px)'
      } else elm.style.width = (it[3] * width) + 'px'
      elm.style.backgroundColor = alpha(color, 0.3 + 0.7 * it[1] / 127)
      elm.dataset.isNote = 'true'
      ;(elm as any).note = it
      cur.appendChild(elm)
    })
  }, [data, width, color])

  useEffect(() => {
    if (!ref.current) return
    const fn = () => {
      if (!grabbing) return
      grabbing = false
      if (ref.current) ref.current.style.cursor = ''
    }
    document.addEventListener('mouseup', fn)
    return () => document.removeEventListener('mouseup', fn)
  }, [ref.current])

  return (
    <div
      ref={ref}
      className='notes'
      onMouseDown={e => {
        if (!scrollableRef.current) return
        let elm = e.target as HTMLDivElement
        if (elm.dataset.isNote) {
          const rect = elm.getBoundingClientRect()
          const currentX = e.pageX - rect.left
          resizeDirection = currentX <= 3 ? -1 : currentX >= rect.width - 3 ? 1 : 0
        } else {
          e.currentTarget.style.cursor = 'grabbing'
          const { top, left } = e.currentTarget.getBoundingClientRect()
          elm = document.createElement('div')
          elm.style.top = ((e.pageY - top) / height | 0) / 1.32 + '%'
          elm.style.left = Math.round((e.pageX - left) / alignmentWidth) * alignmentWidth + 'px'
          elm.style.width = alignmentWidth + 'px'
          elm.style.backgroundColor = alpha(color, 0.3 + 0.7 * 80 / 127)
          elm.dataset.isNote = 'true'
          ;(elm as any).note = []
          e.currentTarget.appendChild(elm)
          resizeDirection = 0
        }
        selectedNotes.forEach(it => (it.className = ''))
        selectedNotes = [elm]
        elm.className = 'selected'
        const rect = e.currentTarget.getBoundingClientRect()
        startX = e.pageX - rect.left
        startY = e.pageY - rect.top
        offsetX = offsetY = 0
        e.currentTarget.style.cursor = resizeDirection ? 'e-resize' : 'grabbing'
        grabbing = true
      }}
      onMouseMove={e => {
        if (!grabbing || !selectedNotes.length) return
        const rect = e.currentTarget.getBoundingClientRect()
        const left = Math.round((e.pageX - rect.left - startX) / alignmentWidth)
        const dx = (left - offsetX) * alignmentWidth
        offsetX = left
        if (resizeDirection) {
          if (!dx) return
          selectedNotes.forEach(it => {
            if (resizeDirection === 1) it.style.width = parseFloat(it.style.width) + dx + 'px'
            else {
              it.style.width = parseFloat(it.style.width) - dx + 'px'
              it.style.left = parseFloat(it.style.left) + dx + 'px'
            }
          })
        } else {
          const top = Math.round((e.pageY - rect.top - startY) / height)
          const dy = (top - offsetY) / 1.32
          offsetY = top
          if (!dx && !dy) return
          selectedNotes.forEach(it => {
            if (dx) it.style.left = parseFloat(it.style.left) + dx + 'px'
            if (dy) it.style.top = parseFloat(it.style.top) + dy + '%'
          })
        }
      }}
    />
  )
}

const Editor: React.FC = () => {
  const [noteWidthLevel, setNoteWidthLevel] = useState(3)
  const [noteHeight] = useState(14)
  const [state] = useGlobalData()
  const [alignment] = useState(state.ppq)
  const editorRef = useRef<HTMLElement | null>(null)
  const noteWidth = noteWidths[noteWidthLevel]
  barLength = noteWidth * state.ppq
  const beatWidth = barLength / (16 / state.timeSigDenominator)

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

  useEffect(() => {
    if (scrollableRef.current) scrollableRef.current.scrollTop = noteHeight * 50
  }, [scrollableRef.current])

  return (
    <div className='editor'>
      <Box className='actions' sx={{ backgroundColor: theme => theme.palette.background.bright }}>
        <Slider
          min={0}
          max={noteWidths.length - 1}
          value={noteWidthLevel}
          className='scale-slider'
          onChange={(_, val) => setNoteWidthLevel(val as number)}
        />
        当前轨道: {state.tracks.find(it => it.uuid === state.activeTrack)?.name || '未选中'}
      </Box>
      <Paper square elevation={3} className='scrollable' ref={scrollableRef as any}>
        <PlayRuler
          headRef={playHeadRef}
          noteWidth={noteWidth}
          movableRef={editorRef}
          onWidthLevelChange={v => setNoteWidthLevel(Math.max(Math.min(noteWidthLevel + (v ? -1 : 1), noteWidths.length - 1), 0))}
        />
        <div className='wrapper'>
          <Paper
            square
            elevation={6}
            className='keyboard'
            sx={{ '& button': { height: noteHeight }, '& button:not(:nth-of-type(12n+1))': { fontSize: noteHeight >= 14 ? undefined : '0!important' } }}
          >
            {keys}
          </Paper>
          <Box
            ref={editorRef}
            className='notes-wrapper'
            sx={{
              '& .notes div': {
                boxShadow: theme => theme.shadows[1],
                backgroundColor: theme => theme.palette.primary.main,
                height: noteHeight
              }
            }}
          >
            <div style={{ width: (state.maxNoteTime + state.ppq * 4) * noteWidth, height: '100%' }}>
              <EditorGrid width={beatWidth} height={noteHeight} timeSigNumerator={state.timeSigNumerator} timeSigDenominator={state.timeSigDenominator} />
              <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
                <rect fill='url(#editor-grid-y)' x='0' y='0' width='100%' height='100%' />
                <rect fill='url(#editor-grid-x)' x='0' y='0' width='100%' height='100%' />
              </svg>
              <Notes
                ppq={state.ppq}
                width={noteWidth}
                height={noteHeight}
                alignment={alignment}
                data={state.trackMidiData[state.activeTrack]?.notes}
                color={state.tracks[$client.trackNameToIndex[state.activeTrack]]?.color || ''}
              />
            </div>
          </Box>
        </div>
      </Paper>
    </div>
  )
}

const BottomBar: React.FC = () => {
  return (
    <Resizable enable={{ top: true }} className='bottom-bar' maxHeight='80vh' minHeight={0}>
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
