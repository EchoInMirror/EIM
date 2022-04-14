import './Editor.less'
import React, { useEffect, useRef, useState, createRef, useMemo, memo } from 'react'
import packets from '../../packets'
import useGlobalData from '../reducer'
import PlayRuler, { moveScrollbar } from './PlayRuler'
import { Paper, Button, Box, useTheme, lighten, alpha, Slider, FormControl, Menu, MenuItem, Select, InputLabel, Divider, ListItemIcon, ListItemText, Typography } from '@mui/material'

import ContentCopy from '@mui/icons-material/ContentCopy'
import ContentCut from '@mui/icons-material/ContentCut'
import ContentPaste from '@mui/icons-material/ContentPaste'

export let barLength = 0
export const playHeadRef = createRef<HTMLDivElement>()

const keyNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
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
let selectedNotes: HTMLDivElement[] = []
let pressedKeys: number[] = []
let activeNote: HTMLDivElement | undefined

let _alignment = 0

const copiedData: packets.IMidiMessage[] = []

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
              const key = (((noteOn.data! >> 8) & 0xff) - offsetY) << 8
              midi.push(
                { time: noteOn.time! + dx, data: (noteOn.data! & 0xff) | key | (noteOn.data! >> 16 & 0xff << 16) },
                { time: noteOff.time! + dx + resizeDirection * offsetX * _alignment, data: (noteOff.data! & 0xff) | key | (noteOff.data! >> 16 << 16) }
              )
            })
            $client.rpc.editMidiMessages({ uuid, data, midi })
          }
          break
        }
        case MouseState.Selecting: if (selectedBoxRef.current) {
          selectedNotes = []
          for (const it of ref.current.children) if (it.className) selectedNotes.push(it as HTMLDivElement)
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

  // const addNotes = (notes: TrackMidiNoteData[]) => {
  //   selectedNotes.forEach(it => (it.className = ''))
  //   if (ref.current) {
  //     selectedNotes = notes.map(it => {
  //       data.push(it)
  //       const elm = createMidiElement(it)
  //       elm.className = 'selected'
  //       ref.current!.appendChild(elm)
  //       return elm
  //     })
  //     data.sort((a, b) => a[2] - b[2])
  //     // $client.addMidiNotes(index, notes)
  //     // $client.trackUpdateNotifier[uuid]?.()
  //   }
  // }

  const close = () => setContextMenu(undefined)
  // const copyNotes = () => {
  //   const left = (selectedNotes.reduce((prev, { note }) => prev > note[2] ? note[2] : prev, Infinity) / alignment | 0) * alignment
  //   copiedData = selectedNotes.map(({ note }) => [note[0], note[1], note[2] - left, note[3]])
  // }
  const deleteNotes = () => {
    const data: number[] = []
    selectedNotes.forEach(({ dataset: { noteOnIndex, noteIndex } }) => data.push(+noteOnIndex!, +noteIndex!))
    $client.rpc.deleteMidiMessages({ uuid, data: data.sort((a, b) => a - b) })
    selectedNotes = []
  }

  const notes = useMemo(() => {
    if (!midi) return
    const arr: JSX.Element[] = []
    const midiOn: Record<number, number | undefined> = { }
    midi.forEach(({ time, data }, i) => {
      switch (data! & 0xf0) {
        case 0x90: // NoteOn
          midiOn[data! >> 8 & 0xff] = i
          break
        case 0x80: { // NoteOff
          const key = data! >> 8 & 0xff
          const onIndex = midiOn[key]
          if (onIndex !== undefined) {
            const noteOn = midi[onIndex]
            midiOn[key] = undefined
            arr.push((
              <div
                key={i}
                data-note-on-index={onIndex}
                data-note-index={i}
                style={{
                  top: ((1 - key / 132) * 100) + '%',
                  left: noteOn.time! * width,
                  width: (time! - noteOn.time!) * width,
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

  if (!midi) return <div ref={ref} className='notes' style={{ cursor: 'no-drop' }} />

  return (
    <>
      <div
        ref={ref}
        className='notes'
        tabIndex={0}
        onMouseUp={e => e.preventDefault()}
        onContextMenu={e => {
          e.preventDefault()
          // setContextMenu(contextMenu ? undefined : { left: e.clientX - 2, top: e.clientY - 4 })
        }}
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
          let elm = e.target as HTMLDivElement
          switch (e.button) {
            case 0: {
              if (elm.dataset.noteIndex) {
                const rect = elm.getBoundingClientRect()
                const currentX = e.pageX - rect.left
                resizeDirection = currentX <= 3 ? -1 : currentX >= rect.width - 3 ? 1 : 0
                e.currentTarget.style.cursor = 'grabbing'
                if (!selectedNotes.includes(elm)) {
                  selectedNotes.forEach(it => (it.className = ''))
                  selectedNotes = [elm]
                  elm.className = 'selected'
                }
              } else {
                if (elm !== e.currentTarget) return
                const { top, left } = e.currentTarget.getBoundingClientRect()
                const time = ((e.pageX - left) / alignmentWidth | 0) * alignmentWidth / width | 0
                const note = (132 - ((e.pageY - top) / height | 0)) << 8
                $client.rpc.addMidiMessages({ uuid, midi: [{ time, data: 0x90 | note | (80 << 16) }, { time: time + alignment, data: 0x80 | note }] })
                elm = selectedNotes[0]
              }
              activeNote = elm
              if (!resizeDirection) {
                // if (pressedKeys.length) {
                //   $client.rpc.sendMidiMessages({ uuid, data: pressedKeys.map(it => 0x80 | (it << 8) | (70 << 16)) }) // NoteOff
                //   pressedKeys = []
                // }
                // $client.rpc.sendMidiMessages({ // NoteOn
                //   uuid,
                //   data: selectedNotes.map(it => {
                //     if (it.note[2] !== elm.note[2]) return
                //     pressedKeys.push(it.note[0])
                //     return 0x90 | (it.note[0] << 8) | (it.note[1] << 16)
                //   })
                // })
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
              // selectedNotes.forEach(it => it.className && (it.className = ''))
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
                if (dy && pressedKeys.length) {
                  // pressedKeys.forEach(it => $client.midiMessage(index, 0x80, it, 80))
                  pressedKeys = []
                }
                const curNote = activeNote?.dataset?.noteOnIndex ? activeNote?.dataset?.noteOnIndex : -1
                selectedNotes.forEach(it => {
                  if (dx) {
                    const left = parseFloat(it.style.left) + dx
                    it.style.left = left + 'px'
                    // midi[+it.dataset.noteOnIndex!].time = left / width | 0
                  }
                  if (dy) {
                    const top = Math.min(Math.max(parseFloat(it.style.top) + dy / 1.32, 0), 100)
                    it.style.top = top + '%'
                    const keyId = Math.round(1.32 * (100 - top))
                    if (it.dataset.noteOnIndex === curNote) {
                      pressedKeys.push(keyId)
                      // $client.midiMessage(index, 0x90, keyId, it.note[1])
                    }
                    // const note = midi[+it.dataset.noteOnIndex!]
                    // note.time = left / width | 0
                    // note.data! ^= (note.data! >> 8) & 0xff
                    // note.data! |= keyId << 8
                  }
                })
              }
              offsetY = top
              offsetX = left
              // $client.trackUpdateNotifier[uuid]?.()
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
                if (noteOn.time! >= minLeft && noteOn.time! < maxLeft && minTop <= key && maxTop > key) {
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
        // onKeyUp={e => {
        //   const ctrl = e.metaKey || e.ctrlKey
        //   switch (e.code) {
        //     case 'KeyX': {
        //       if (!ctrl) break
        //       copyNotes()
        //     }
        //     // eslint-disable-next-line no-fallthrough
        //     case 'Delete': {
        //       deleteNotes()
        //       break
        //     }
        //     case 'KeyC': {
        //       if (ctrl) copyNotes()
        //       break
        //     }
        //     case 'KeyV': {
        //       if (ctrl && copiedData.length) addNotes(copiedData)
        //       break
        //     }
        //   }
        // }}
      >
        {notes}
        <Menu open={!!contextMenu} onClose={close} anchorReference='anchorPosition' anchorPosition={contextMenu} sx={{ '& .MuiPaper-root': { width: 170 } }}>
          <MenuItem
            disabled={!selectedNotes.length}
            onClick={() => {
              // copyNotes()
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
              // copyNotes()
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
              // if (copiedData.length) addNotes(copiedData)
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
              // const left = (selectedNotes.reduce((prev, { note }) => prev > note[2] ? note[2] : prev, Infinity) / alignment | 0) * alignment
              // const arr = selectedNotes.map(({ note }) => [note[0], note[1], note[2] - left, note[3]])
              // arr.unshift(ppq as any)
              // navigator.clipboard.writeText('EIMNotes' + JSON.stringify(arr))
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
                const arr = JSON.parse(data.slice(8))
                if (!Array.isArray(arr)) return
                const curPPQ = arr.shift()
                if (typeof curPPQ !== 'number' || !arr.length) return
                arr.forEach(it => {
                  it[2] = Math.round(it[2] / curPPQ * ppq)
                  it[3] = Math.round(it[3] / curPPQ * ppq)
                })
                // addNotes(arr)
              }).catch(console.error)
              close()
            }}
          >
            <ListItemIcon><ContentPaste fontSize='small' /></ListItemIcon>
            <ListItemText>从剪辑版粘贴</ListItemText>
          </MenuItem>
        </Menu>
      </div>
    </>
  )
})

let mouseOut = false
let direction = 0
let timer: NodeJS.Timer | undefined
const Editor: React.FC = () => {
  const [noteWidthLevel, setNoteWidthLevel] = useState(3)
  const [noteHeight] = useState(14)
  const [state] = useGlobalData()
  const [alignment, setAlignment] = useState(state.ppq)
  const editorRef = useRef<HTMLElement | null>(null)
  const noteWidth = noteWidths[noteWidthLevel]
  barLength = noteWidth * state.ppq
  const beatWidth = barLength / (16 / state.timeSigDenominator)

  const track = state.activeTrack ? state.tracks[state.activeTrack] : undefined

  useEffect(() => {
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
    <div className='editor'>
      <Box className='actions' sx={{ backgroundColor: theme => theme.palette.background.bright }}>
        <Slider
          min={0}
          max={noteWidths.length - 1}
          value={noteWidthLevel}
          className='scale-slider'
          onChange={(_, val) => setNoteWidthLevel(val as number)}
        />
        当前轨道: {track ? track.name : '未选中'}
        <br />
        <FormControl variant='standard'>
          <InputLabel id='bottom-bar-alignment-label'>对齐</InputLabel>
          <Select
            labelId='bottom-bar-alignment-label'
            id='bottom-bar-alignment'
            value={alignment}
            onChange={e => setAlignment(e.target.value as number)}
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
      </Box>
      <Paper
        square
        elevation={3}
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
            <div style={{ width: (state.maxNoteTime + state.ppq * 4) * noteWidth, height: '100%', minWidth: '100%' }}>
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
      </Paper>
    </div>
  )
}

export default Editor
