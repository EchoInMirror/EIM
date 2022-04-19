import './Editor.less'
import React, { useEffect, useRef, useState, createRef, useMemo, memo } from 'react'
import packets from '../../packets'
import useGlobalData from '../reducer'
import EventEditor from './EventEditor'
import PlayRuler, { moveScrollbar } from './PlayRuler'
import { keyNames } from '../utils'
import {
  Paper, Button, Box, useTheme, lighten, alpha, Slider, FormControl, Menu, MenuItem, Select, InputLabel, Divider, ListItemIcon,
  ListItemText, Typography, Checkbox, TextField, FormControlLabel
} from '@mui/material'

import ContentCopy from '@mui/icons-material/ContentCopy'
import ContentCut from '@mui/icons-material/ContentCut'
import ContentPaste from '@mui/icons-material/ContentPaste'
import Lightbulb from '@mui/icons-material/Lightbulb'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const Keyboard = memo(function Keyboard () {
  const keys: JSX.Element[] = []
  for (let i = 0; i < 132; i++) {
    const name = keyNames[i % 12]
    let elm: string | JSX.Element = name + ' ' + (i / 12 | 0)
    if (!(i % 12)) elm = <b>{elm}</b>
    else if (name.length === 2) elm = <i>{elm}</i>
    keys.push(<Button key={i} data-eim-keyboard-key={i}>{elm}</Button>)
  }
  return <>{keys}</>
})

const noteWidths = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.75, 1, 1.5, 2, 3, 4.5]

const scales = [true, false, true, false, true, true, false, true, false, true, false, true].reverse()

const EditorGrid = memo(function EditorGrid ({ width, height, timeSigNumerator, timeSigDenominator }:
  { width: number, height: number, timeSigNumerator: number, timeSigDenominator: number }) {
  const theme = useTheme()
  const rects = []
  const lines = []
  const rectsX = []
  const highlightColor = alpha(theme.palette.primary.main, 0.1)
  const gridXDeepColor = alpha(theme.palette.divider, 0.26)
  const beats = 16 / timeSigDenominator
  for (let i = 0; i < 12; i++) {
    const y = (i + 1) * height
    rects.push(<rect key={i} height={height} width='81' y={height * i} fill={scales[i] === (theme.palette.mode === 'dark') ? highlightColor : 'none'} />)
    lines.push(<line x1='0' x2='80' y1={y} y2={y} key={i} />)
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
})

const scrollableRef = createRef<HTMLDivElement>()
const selectedBoxRef = createRef<HTMLDivElement>()

let startX = 0
let startY = 0
let offsetY = 0
let offsetX = 0
let resizeDirection = 0
let mouseState = 0
let boxHeight = 0
let boxWidth = 0
let selectedIndexes: Record<number, true | undefined> = {}
let selectedNotes: HTMLDivElement[] = []
let pressedKeys: number[] = []
let activeNote: HTMLDivElement | undefined
let activeIndex = -1

let _alignment = 0

let copiedData: packets.IMidiMessage[] = []

enum MouseState {
  None,
  Dragging,
  Selecting,
  Deleting
}

const Notes = memo(function Notes ({ midi, width, height, ppq, color, alignment, uuid }: {
  midi: packets.IMidiMessage[] | null | undefined
  width: number
  height: number
  ppq: number
  color: string
  alignment: number
  uuid: string
}) {
  const [contextMenu, setContextMenu] = React.useState<{ left: number, top: number } | undefined>()
  const ref = useRef<HTMLDivElement | null>(null)
  const alignmentWidth = width * alignment
  _alignment = alignment

  useEffect(() => {
    const fn = () => {
      if (!mouseState || !ref.current) return
      switch (mouseState) {
        case MouseState.Dragging: {
          const dx = resizeDirection === 1 ? 0 : offsetX * _alignment
          const uuid = $globalData.activeTrack
          const track = $globalData.tracks[uuid]
          if (uuid && track) {
            const data: number[] = []
            const midi: packets.IMidiMessage[] = []
            selectedNotes.forEach(({ dataset: { noteOnIndex, noteIndex } }) => {
              const onIndex = +noteOnIndex!
              const offIndex = +noteIndex!
              const noteOn = track.midi![onIndex]
              const noteOff = track.midi![offIndex]
              data.push(onIndex, offIndex)
              let originKey = (noteOn.data! >> 8) & 0xff
              const key = (originKey - offsetY) << 8
              originKey <<= 8
              midi.push(
                { time: (noteOn.time || 0) + dx, data: (noteOn.data! ^ originKey) | key },
                { time: (noteOff.time || 0) + dx + resizeDirection * offsetX * _alignment, data: (noteOff.data! ^ originKey) | key }
              )
            })
            $client.rpc.editMidiMessages({ uuid, data, midi })
          }
          break
        }
        case MouseState.Selecting: if (selectedBoxRef.current) {
          selectedNotes = []
          selectedIndexes = {}
          for (const it of ref.current.children) {
            if (it.className) {
              selectedNotes.push(it as HTMLDivElement)
              selectedIndexes[+(it as HTMLDivElement).dataset.noteOnIndex!] = true
            }
          }
          $client.emit('editor:selectedNotes', selectedIndexes)
          selectedBoxRef.current.style.display = 'none'
          selectedBoxRef.current.style.width = '0'
          selectedBoxRef.current.style.height = '0'
        }
      }
      mouseState = MouseState.None
      ref.current.style.cursor = ''
    }
    document.addEventListener('mouseup', fn)
    return () => document.removeEventListener('mouseup', fn)
  }, [])

  useEffect(() => {
    if (!ref.current || !midi) return
    selectedNotes = []
    const selectedIndexes2: typeof selectedIndexes = {}
    for (const elm of ref.current!.children) {
      const it = elm as HTMLDivElement
      if (!it.dataset.noteOnIndex) continue
      const curId = +it.dataset.noteOnIndex!
      const oldIndex: number = (midi![curId] as any)?._oldIndex
      if (selectedIndexes![oldIndex]) {
        selectedNotes.push(it)
        selectedIndexes2[curId] = true
        it.className = 'selected'
        if (oldIndex === activeIndex) activeNote = it
      } else if (it.className) it.className = ''
    }
    activeIndex = -1
    selectedIndexes = selectedIndexes2
    $client.emit('editor:selectedNotes', selectedIndexes)
  }, [midi, ref.current])

  const notes = useMemo(() => {
    if (!midi) return
    const arr: JSX.Element[] = []
    const midiOn: Record<number, number | number[] | undefined> = {}
    midi.forEach(({ time, data }, i) => {
      const key = data! >> 8 & 0xff
      switch (data! & 0xf0) {
        case 0x90: { // NoteOn
          const val = midiOn[key]
          if (val === undefined) midiOn[key] = i
          else if (typeof val === 'number') midiOn[key] = [val, i]
          else val.push(i)
          break
        }
        case 0x80: { // NoteOff
          const onIndex = midiOn[key]
          if (onIndex !== undefined) {
            const onlyOne = typeof onIndex === 'number'
            const trueOnIndex = onlyOne ? onIndex : onIndex.shift()!
            const noteOn = midi[trueOnIndex]
            if (onlyOne) midiOn[key] = undefined
            else if ((onIndex as number[]).length === 1) midiOn[key] = onIndex[0]
            arr.push((
              <div
                key={i}
                data-note-on-index={trueOnIndex}
                data-note-index={i}
                style={{
                  top: ((1 - key / 132) * 100) + '%',
                  left: (noteOn.time || 0) * width,
                  width: ((time || 0) - (noteOn.time || 0)) * width,
                  backgroundColor: lighten(color, 0.3 + 0.7 * (noteOn.data! >> 24 & 0xff) / 127)
                }}
              />
            ))
          }
        }
      }
    })
    return arr
  }, [midi, width])

  if (!midi) return <div ref={ref} className='notes' />

  const close = () => setContextMenu(undefined)
  const copyNotes = () => {
    if (!selectedNotes.length) return
    const left = selectedNotes.reduce((prev, { dataset: { noteOnIndex } }) => {
      const time = midi[+noteOnIndex!].time || 0
      return prev > time ? time : prev
    }, Infinity)
    copiedData = []
    selectedNotes.forEach(({ dataset: { noteOnIndex, noteIndex } }) => {
      const noteOn = midi[+noteOnIndex!]
      const noteOff = midi[+noteIndex!]
      copiedData.push({ time: (noteOn.time || 0) - left, data: noteOn.data }, { time: (noteOff.time || 0) - left, data: noteOff.data })
    })
  }
  const deleteNotes = () => {
    const data: number[] = []
    selectedNotes.forEach(({ dataset: { noteOnIndex, noteIndex } }) => data.push(+noteOnIndex!, +noteIndex!))
    $client.rpc.deleteMidiMessages({ uuid, data: data.sort((a, b) => a - b) })
    selectedNotes = []
    selectedIndexes = {}
    $client.emit('editor:selectedNotes', selectedIndexes)
  }
  const addNotes = (data: packets.IMidiMessage[]) => {
    $client.rpc.addMidiMessages({ uuid, midi: data })
    selectedNotes = []
    selectedIndexes = {}
    const len = midi.length
    data.forEach((it, i) => {
      if ((it.data! & 0xf0) === 0x90) selectedIndexes[len + i] = true
    })
    $client.emit('editor:selectedNotes', selectedIndexes)
    return len
  }

  return (
    <div
      ref={ref}
      className='notes'
      tabIndex={0}
      onMouseUp={e => e.preventDefault()}
      onContextMenu={e => e.preventDefault()}
      onCopy={e => console.log(e.clipboardData)}
      onMouseDown={e => {
        if (!scrollableRef.current) return
        const rect = e.currentTarget.getBoundingClientRect()
        startX = e.pageX - rect.left
        startY = e.pageY - rect.top
        offsetX = offsetY = 0
        if (mouseState === MouseState.Selecting && selectedBoxRef.current) {
          selectedBoxRef.current.style.display = 'none'
          selectedBoxRef.current.style.width = '0'
          selectedBoxRef.current.style.height = '0'
        }
        if (e.button === 4 || e.ctrlKey) {
          if (!selectedBoxRef.current) return
          mouseState = MouseState.Selecting
          boxHeight = boxWidth = 0
          selectedBoxRef.current.style.left = (startX / alignmentWidth | 0) * alignmentWidth + 'px'
          selectedBoxRef.current.style.top = (startY / height | 0) * height + 'px'
          selectedBoxRef.current.style.display = 'block'
          return
        }
        const elm = e.target as HTMLDivElement
        switch (e.button) {
          case 0: {
            const { noteOnIndex } = elm.dataset
            let activeNoteKey = -1
            if (noteOnIndex) {
              const rect = elm.getBoundingClientRect()
              const currentX = e.pageX - rect.left
              resizeDirection = currentX <= 3 ? -1 : currentX >= rect.width - 3 ? 1 : 0
              e.currentTarget.style.cursor = 'grabbing'
              if (!selectedNotes.includes(elm)) {
                selectedNotes.forEach(it => (it.className = ''))
                selectedNotes = [elm]
                selectedIndexes = { [activeIndex = +elm.dataset.noteOnIndex!]: true }
                elm.className = 'selected'
              }
              activeNote = elm
            } else {
              if (elm !== e.currentTarget) return
              const { top, left } = e.currentTarget.getBoundingClientRect()
              const time = ((e.pageX - left) / alignmentWidth | 0) * alignmentWidth / width | 0
              let note = activeNoteKey = (132 - ((e.pageY - top) / height | 0))
              selectedNotes = []
              activeNote = undefined
              activeIndex = addNotes([{ time, data: 0x90 | (note <<= 8) | (80 << 16) }, { time: time + alignment, data: 0x80 | note }])
            }
            if (!$globalData.isPlaying && !resizeDirection) {
              if (pressedKeys.length) {
                $client.rpc.sendMidiMessages({ uuid, data: pressedKeys.map(it => 0x80 | (it << 8)) }) // NoteOff
                pressedKeys = []
              }
              if (activeNoteKey === -1) {
                const time = midi[+noteOnIndex!].time
                const arr: number[] = []
                selectedNotes.forEach(it => {
                  const { time: curTime, data } = midi[+it.dataset.noteOnIndex!]
                  if (curTime !== time) return
                  pressedKeys.push((data! >> 8) & 0xff)
                  arr.push(data!)
                })
                $client.rpc.sendMidiMessages({ uuid, data: arr })
              } else {
                pressedKeys.push(activeNoteKey)
                $client.rpc.sendMidiMessages({ uuid, data: [0x90 | (activeNoteKey << 8) | (80 << 16)] })
              }
            }
            e.currentTarget.style.cursor = resizeDirection ? 'e-resize' : 'grabbing'
            mouseState = MouseState.Dragging
            break
          }
          case 1:
            e.preventDefault()
            setContextMenu(contextMenu ? undefined : { left: e.clientX - 2, top: e.clientY - 4 })
            break
          case 2:
            e.preventDefault()
            if (elm.dataset.noteIndex) $client.rpc.deleteMidiMessages({ uuid, data: [+elm.dataset.noteOnIndex!, +elm.dataset.noteIndex].sort((a, b) => a - b) })
            selectedNotes = []
            mouseState = MouseState.Deleting
            break
        }
      }}
      onMouseMove={e => {
        if (!mouseState) return
        const rect = e.currentTarget.getBoundingClientRect()
        switch (mouseState) {
          case MouseState.Dragging: {
            if (!selectedNotes.length) break
            const left = Math.round((e.pageX - rect.left - startX) / alignmentWidth)
            const top = Math.round((e.pageY - rect.top - startY) / height)
            if (left === offsetX && top === offsetY) return
            const dx = (left - offsetX) * alignmentWidth
            const dy = top - offsetY
            if (resizeDirection) {
              if (!dx || selectedNotes.some(it => resizeDirection === 1
                ? dx < 0 && parseFloat(it.style.width) < alignmentWidth
                : dx > 0 && parseFloat(it.style.width) < alignmentWidth)) return
              selectedNotes.forEach(it => {
                if (resizeDirection === 1) it.style.width = parseFloat(it.style.width) + dx + 'px'
                else {
                  it.style.width = parseFloat(it.style.width) - dx + 'px'
                  it.style.left = parseFloat(it.style.left) + dx + 'px'
                }
              })
            } else {
              if (selectedNotes.some(it => {
                if (dx && parseFloat(it.style.left) + dx < -0.005) return true
                if (dy) {
                  const tmp = parseFloat(it.style.top) + dy / 1.32
                  if (tmp < -0.005 || tmp >= 100) return true
                }
                return false
              })) return
              if (!$globalData.isPlaying && dy && pressedKeys.length) {
                $client.rpc.sendMidiMessages({ uuid, data: pressedKeys.map(it => 0x80 | (it << 8) | (70 << 16)) }) // NoteOff
                pressedKeys = []
              }
              const curNote = activeNote?.dataset?.noteOnIndex ? midi[+activeNote.dataset.noteOnIndex].time! : -1
              const data: number[] = []
              selectedNotes.forEach(it => {
                if (dx) {
                  const left = parseFloat(it.style.left) + dx
                  it.style.left = left + 'px'
                }
                if (dy) {
                  const top = Math.min(Math.max(parseFloat(it.style.top) + dy / 1.32, 0), 100)
                  it.style.top = top + '%'
                  const keyId = Math.round(1.32 * (100 - top))
                  const note = midi[+it.dataset.noteOnIndex!]
                  if (!$globalData.isPlaying && note.time === curNote) {
                    pressedKeys.push(keyId)
                    data.push((note.data! ^ ((note.data! >> 8) & 0xff) << 8) | (keyId << 8))
                  }
                }
              })
              if (!$globalData.isPlaying && dy) $client.rpc.sendMidiMessages({ uuid, data })
            }
            offsetY = top
            offsetX = left
            break
          }
          case MouseState.Selecting: {
            if (!selectedBoxRef.current) break
            const left0 = Math.ceil((e.pageX - rect.left - startX) / alignmentWidth)
            const top0 = Math.ceil((e.pageY - rect.top - startY) / height)
            if (left0 === offsetX && top0 === offsetY) return
            boxWidth = boxWidth + (left0 - offsetX) * alignmentWidth
            boxHeight = boxHeight + (top0 - offsetY) * height
            offsetY = top0
            offsetX = left0
            const box = selectedBoxRef.current.style
            const left = parseFloat(box.left)
            const top = parseFloat(box.top)
            const minLeft0 = Math.round(left / width)
            const maxLeft0 = Math.round((left + boxWidth) / width)
            const minTop0 = Math.round(top / height)
            const maxTop0 = Math.round((top + boxHeight) / height)
            const minLeft = Math.min(minLeft0, maxLeft0)
            const maxLeft = Math.max(minLeft0, maxLeft0)
            const minTop = Math.min(minTop0, maxTop0)
            const maxTop = Math.max(minTop0, maxTop0)
            box.width = Math.abs(boxWidth) + 1 + 'px'
            box.height = Math.abs(boxHeight) + 'px'
            box.transform = `translate(${boxWidth < 0 ? boxWidth : 0}px, ${boxHeight < 0 ? boxHeight : 0}px)`
            for (const elm of e.currentTarget.children as any as HTMLDivElement[]) {
              const { noteOnIndex } = elm.dataset
              if (!noteOnIndex) return
              const noteOn = midi[+noteOnIndex]
              const key = 132 - ((noteOn.data! >> 8) & 0xff)
              if ((noteOn.time || 0) >= minLeft && (noteOn.time || 0) < maxLeft && minTop <= key && maxTop > key) {
                elm.className = 'selected'
              } else if (elm.className) elm.className = ''
            }
            break
          }
          case MouseState.Deleting: {
            const { dataset } = e.target as HTMLDivElement
            if (dataset.noteIndex) $client.rpc.deleteMidiMessages({ uuid, data: [+dataset.noteOnIndex!, +dataset.noteIndex!].sort((a, b) => a - b) })
          }
        }
      }}
      onKeyUp={e => {
        const ctrl = e.metaKey || e.ctrlKey
        switch (e.code) {
          case 'KeyX': {
            if (!ctrl) break
            copyNotes()
          }
          // eslint-disable-next-line no-fallthrough
          case 'Delete': {
            deleteNotes()
            break
          }
          case 'KeyC': {
            if (ctrl) copyNotes()
            break
          }
          case 'KeyV': {
            if (ctrl && copiedData.length) addNotes(copiedData)
            break
          }
        }
      }}
    >
      {notes}
      <Menu open={!!contextMenu} onClose={close} anchorReference='anchorPosition' anchorPosition={contextMenu} sx={{ '& .MuiPaper-root': { width: 170 } }}>
        <MenuItem
          disabled={!selectedNotes.length}
          onClick={() => {
            copyNotes()
            deleteNotes()
            close()
          }}
        >
          <ListItemIcon><ContentCut fontSize='small' /></ListItemIcon>
          <ListItemText>剪切</ListItemText>
          <Typography variant='body2' color='text.secondary'>⌘X</Typography>
        </MenuItem>
        <MenuItem
          disabled={!selectedNotes.length}
          onClick={() => {
            copyNotes()
            close()
          }}
        >
          <ListItemIcon><ContentCopy fontSize='small' /></ListItemIcon>
          <ListItemText>复制</ListItemText>
          <Typography variant='body2' color='text.secondary'>⌘C</Typography>
        </MenuItem>
        <MenuItem
          disabled={!copiedData.length}
          onClick={() => {
            if (copiedData.length) addNotes(copiedData)
            close()
          }}
        >
          <ListItemIcon><ContentPaste fontSize='small' /></ListItemIcon>
          <ListItemText>粘贴</ListItemText>
          <Typography variant='body2' color='text.secondary'>⌘V</Typography>
        </MenuItem>
        <MenuItem
          disabled={!selectedNotes.length}
          onClick={() => {
            copyNotes()
            const data = [...copiedData]
            data.unshift(ppq as any)
            navigator.clipboard.writeText('EIMNotes' + JSON.stringify(data))
            close()
          }}
        >
          <ListItemIcon><ContentCopy fontSize='small' /></ListItemIcon>
          <ListItemText>复制到剪辑版</ListItemText>
        </MenuItem>
        <MenuItem
          onClick={() => {
            navigator.clipboard.readText().then(data => {
              if (!data.startsWith('EIMNotes')) return
              const arr: packets.IMidiMessage[] = JSON.parse(data.slice(8))
              if (!Array.isArray(arr)) return
              const curPPQ = arr.shift() as any as number
              if (typeof curPPQ !== 'number' || !arr.length) return
              arr.forEach(it => (it.time = Math.round((it.time || 0) / curPPQ * ppq)))
              addNotes(arr)
            }).catch(console.error)
            close()
          }}
        >
          <ListItemIcon><ContentPaste fontSize='small' /></ListItemIcon>
          <ListItemText>从剪辑版粘贴</ListItemText>
        </MenuItem>
        <MenuItem>
          <ListItemIcon><Lightbulb fontSize='small' /></ListItemIcon>
          <ListItemText>AI 续写</ListItemText>
        </MenuItem>
      </Menu>
    </div>
  )
})

let mouseOut = false
let direction = 0
let timer: NodeJS.Timer | undefined
const Editor: React.FC = () => {
  const [noteWidthLevel, setNoteWidthLevel] = useState(3)
  const [noteHeight] = useState(14)
  const [state] = useGlobalData()
  const [alignment, setAlignment] = useState(() => +localStorage.getItem('eim:editor:alignment')! || state.ppq)
  const editorRef = useRef<HTMLElement | null>(null)
  const eventEditorRef = useRef<HTMLDivElement | null>(null)
  const noteWidth = noteWidths[noteWidthLevel]
  barLength = noteWidth * state.ppq
  const beatWidth = barLength / (16 / state.timeSigDenominator)
  const contentWidth = (state.maxNoteTime + state.ppq * 4) * noteWidth

  const track = state.activeTrack ? state.tracks[state.activeTrack] : undefined

  useEffect(() => () => {
    clearInterval(timer!)
    timer = undefined
  }, [])

  useEffect(() => {
    const mousedown = (e: MouseEvent) => {
      const btn = e.target as HTMLButtonElement
      const key = btn?.dataset?.eimKeyboardKey
      if (!key || !$globalData.activeTrack) return
      const rect = btn.getBoundingClientRect()
      const keyId = +key & 127
      pressedKeys.push(keyId)
      $client.rpc.sendMidiMessages({
        uuid: $globalData.activeTrack,
        data: [0x90 | (keyId << 8) | (Math.min(rect.width, Math.max(0, e.pageX - rect.left)) / rect.width * 127 << 16)]
      })
    }
    const mouseup = () => {
      clearInterval(timer!)
      timer = undefined
      if (!pressedKeys.length || !$globalData.activeTrack) return
      $client.rpc.sendMidiMessages({
        uuid: $globalData.activeTrack,
        data: pressedKeys.map(it => 0x80 | (it << 8))
      })
      pressedKeys = []
    }
    document.addEventListener('mousedown', mousedown)
    document.addEventListener('mouseup', mouseup)
    return () => {
      document.removeEventListener('mousedown', mousedown)
      document.removeEventListener('mouseup', mouseup)
    }
  }, [])

  useEffect(() => {
    if (scrollableRef.current) scrollableRef.current.scrollTop = noteHeight * 50
  }, [scrollableRef.current])

  const steps = state.ppq / 16 * state.timeSigDenominator

  return (
    <div className='editor' style={{ cursor: track ? undefined : 'no-drop' }}>
      <Box className='actions' sx={{ backgroundColor: theme => theme.palette.background.bright }}>
        <Slider
          min={0}
          max={noteWidths.length - 1}
          value={noteWidthLevel}
          className='scale-slider'
          onChange={(_, val) => setNoteWidthLevel(val as number)}
        />
        <div className='left-wrapper'>
          当前轨道: {track ? track.name : '未选中'}
          <br />
          <br />
          <Box display='flex'>
            <FormControl variant='standard' style={{ flex: 1 }}>
              <InputLabel id='bottom-bar-alignment-label'>对齐</InputLabel>
              <Select
                labelId='bottom-bar-alignment-label'
                id='bottom-bar-alignment'
                value={alignment}
                onChange={e => {
                  const val = e.target.value as number
                  localStorage.setItem('eim:editor:alignment', val.toString())
                  setAlignment(val)
                }}
                label='对齐'
              >
                <MenuItem value={state.ppq * state.timeSigNumerator}><em>小节</em></MenuItem>
                <Divider />
                <MenuItem value={state.ppq}><em>节拍</em></MenuItem>
                <MenuItem value={state.ppq / 2}>1/2 拍</MenuItem>
                <MenuItem value={state.ppq / 3 | 0}>1/3 拍</MenuItem>
                <MenuItem value={state.ppq / 4}>1/4 拍</MenuItem>
                <MenuItem value={state.ppq / 6 | 0}>1/6 拍</MenuItem>
                <Divider />
                <MenuItem value={steps}><em>步进</em></MenuItem>
                <MenuItem value={steps / 2}>1/2 步</MenuItem>
                <MenuItem value={steps / 3 | 0}>1/3 步</MenuItem>
                <MenuItem value={steps / 4}>1/4 步</MenuItem>
                <MenuItem value={steps / 6 | 0}>1/6 步</MenuItem>
                <Divider />
                <MenuItem value={1}><em>无</em></MenuItem>
              </Select>
            </FormControl>
            &nbsp;
            <TextField
              id='standard-number'
              label='默认力度'
              type='number'
              defaultValue={100}
              style={{ width: 60 }}
              InputLabelProps={{
                shrink: true
              }}
              variant='standard'
            />
          </Box>

          <FormControlLabel
            id='checkbox'
            value='top'
            control={<Checkbox defaultChecked />}
            label='试听音符'
            labelPlacement='start'
          />
        </div>
      </Box>

      <Paper square className='right-wrapper' elevation={3}>
        <div
          className='scrollable'
          ref={scrollableRef as any}
          onMouseOut={() => (mouseOut = true)}
          onMouseMove={e => {
            if (!mouseState) return
            mouseOut = false
            const rect = e.currentTarget.getBoundingClientRect()
            if (e.pageX - rect.left < 64) {
              direction |= 0b0001
              direction &= ~0b0010
            } else if (rect.right - e.pageX < 16) {
              direction &= ~0b0001
              direction |= 0b0010
            } else direction &= ~0b0011
            if (e.pageY - rect.top < 40) {
              direction |= 0b0100
              direction &= ~0b1000
            } else if (rect.bottom - e.pageY < 16) {
              direction &= ~0b0100
              direction |= 0b1000
            } else direction &= ~0b1100
            if (direction && timer == null) {
              timer = setInterval(() => {
                if (!direction || !scrollableRef.current) {
                  clearInterval(timer!)
                  timer = undefined
                  return
                }
                if (direction & 0b0001) moveScrollbar('bottom-bar', mouseOut ? -4 : -2)
                else if (direction & 0b0010) moveScrollbar('bottom-bar', mouseOut ? 4 : 2)
                if (direction & 0b0100) scrollableRef.current.scrollTop -= mouseOut ? 8 : 4
                else if (direction & 0b1000) scrollableRef.current.scrollTop += mouseOut ? 8 : 4
              }, 30)
            }
          }}
        >
          <PlayRuler
            id='bottom-bar'
            headRef={playHeadRef}
            noteWidth={noteWidth}
            movableRef={[editorRef, eventEditorRef]}
            onWidthLevelChange={v => setNoteWidthLevel(Math.max(Math.min(noteWidthLevel + (v ? -1 : 1), noteWidths.length - 1), 0))}
          />
          <div className='wrapper'>
            <Paper
              square
              elevation={6}
              className='keyboard'
              sx={{ '& button': { height: noteHeight }, '& button:not(:nth-of-type(12n+1))': { fontSize: noteHeight >= 14 ? undefined : '0!important' } }}
            >
              <Keyboard />
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
              <div style={{ width: contentWidth, height: '100%', minWidth: '100%' }}>
                <EditorGrid width={beatWidth} height={noteHeight} timeSigNumerator={state.timeSigNumerator} timeSigDenominator={state.timeSigDenominator} />
                <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
                  <rect fill='url(#editor-grid-y)' x='0' y='0' width='100%' height='100%' />
                  <rect fill='url(#editor-grid-x)' x='0' y='0' width='100%' height='100%' />
                </svg>
                <Box
                  className='selected-box'
                  ref={selectedBoxRef}
                  sx={{ borderColor: theme => theme.palette.primary.main, backgroundColor: theme => alpha(theme.palette.primary.main, 0.2) }}
                />
                <Notes
                  ppq={state.ppq}
                  width={noteWidth}
                  height={noteHeight}
                  alignment={alignment}
                  midi={track?.midi}
                  uuid={state.activeTrack || ''}
                  color={track?.color || ''}
                />
              </div>
            </Box>
          </div>
        </div>
        <EventEditor
          midi={track?.midi}
          color={track?.color || ''}
          beatWidth={beatWidth}
          noteWidth={noteWidth}
          contentWidth={contentWidth}
          eventEditorRef={eventEditorRef}
          timeSigNumerator={state.timeSigNumerator}
          timeSigDenominator={state.timeSigDenominator}
        />
      </Paper>
    </div>
  )
}

export default Editor
