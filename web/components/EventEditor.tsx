import './EventEditor.less'
import packets from '../../packets'
import React, { useMemo, memo, useEffect, useRef, useState } from 'react'
import { Resizable } from 're-resizable'
import { Paper, Box, useTheme, alpha, Select, MenuItem, Button, CircularProgress } from '@mui/material'
import IMidiMessages = EIMPackets.IMidiMessages

const EventEditorGrid = memo(function EventEditorGrid ({
  width,
  timeSigNumerator,
  timeSigDenominator
}:
  { width: number, timeSigNumerator: number, timeSigDenominator: number }) {
  const theme = useTheme()
  const rectsX = []
  const gridXDeepColor = alpha(theme.palette.divider, 0.26)
  const beats = 16 / timeSigDenominator
  for (let j = 0, cur = 0; j < timeSigNumerator; j++) {
    rectsX.push(<rect width='1' height='3240' x={width * cur++} y='0'
      fill={j ? gridXDeepColor : alpha(theme.palette.divider, 0.44)} key={j << 5}
                />)
    if (width >= 9.6) {
      for (let i = 1; i < beats; i++) {
        rectsX.push(<rect width='1' height='3240' x={width * cur++} y='0' key={j << 5 | i} />)
      }
    } else {
      cur += beats - 1
    }
  }
  return (
    <svg xmlns='http://www.w3.org/2000/svg' width='0' height='0' style={{ position: 'absolute' }}>
      <defs>
        <pattern id='event-editor-grid-x' x='0' y='0' width={width * beats * timeSigNumerator} height='3240'
          patternUnits='userSpaceOnUse'
        >
          <g fill={theme.palette.divider}>{rectsX}</g>
        </pattern>
      </defs>
    </svg>
  )
})

let startY = 0
let wrapperHeight = 0
let hasSelected = false
let targetElm: HTMLDivElement | undefined
let _midi: packets.IMidiMessage[] | null | undefined
const Velocity = memo(function Velocity ({
  midi,
  color,
  noteWidth
}: {
  midi: packets.IMidiMessage[] | null | undefined
  color: string
  noteWidth: number
}) {
  const ref = useRef<HTMLDivElement>()
  _midi = midi

  useEffect(() => {
    const fn = (indexes: Record<number, true | undefined>) => {
      hasSelected = false
      if (!ref.current) return
      for (const node of ref.current.children) {
        const id = +(node as any).dataset.noteIndex!
        if (node.className) {
          if (indexes[id]) {
            hasSelected = true
          } else {
            node.className = ''
          }
        } else if (indexes[id]) {
          node.className = 'selected'
          hasSelected = true
        }
      }
    }
    $client.on('editor:selectedNotes', fn)
    const handleMouseUp = () => {
      if (!targetElm || !ref.current || !_midi || !$globalData.activeTrack) return
      const id = +targetElm.dataset.noteIndex!
      const data = [id]
      const midi: packets.IMidiMessage[] = [{
        time: _midi[id].time || 0,
        data: (_midi[id].data! & 0xffff) | (Math.round(parseFloat(targetElm.style.height) * 1.27) << 16)
      }]
      for (const node of ref.current.children) {
        const it = node as HTMLDivElement
        if (it !== targetElm && node.className) {
          const id = +it.dataset.noteIndex!
          data.push(id)
          midi.push({
            time: _midi[id].time || 0,
            data: (_midi[id].data! & 0xffff) | (Math.round(parseFloat(it.style.height) * 1.27) << 16)
          })
        }
      }
      targetElm = undefined
      $client.rpc.editMidiMessages({
        uuid: $globalData.activeTrack,
        data,
        midi
      })
    }
    const handleMouseMove = (e: MouseEvent) => {
      if (!targetElm || !ref.current || !_midi) return
      for (const node of ref.current.children) {
        const it = node as HTMLDivElement
        if (it !== targetElm && node.className) {
          const id = +it.dataset.noteIndex!
          it.style.height = Math.min(Math.max(((_midi[id].data! >> 16) & 0xff) + Math.round((startY - e.pageY) / wrapperHeight * 127), 0), 127) / 1.27 + '%'
        }
      }
      const id = +targetElm.dataset.noteIndex!
      targetElm.style.height = Math.min(Math.max(((_midi[id].data! >> 16) & 0xff) + Math.round((startY - e.pageY) / wrapperHeight * 127), 0), 127) / 1.27 + '%'
    }
    document.addEventListener('mouseup', handleMouseUp)
    document.addEventListener('mousemove', handleMouseMove)
    return () => {
      $client.off('editor:selectedNotes', fn)
      document.removeEventListener('mouseup', handleMouseUp)
      document.removeEventListener('mousemove', handleMouseMove)
    }
  }, [])

  const notes = useMemo(() => {
    if (!midi) return
    const arr: JSX.Element[] = []
    midi.forEach(({
      time,
      data
    }, i) => {
      if ((data! & 0xf0) === 0x90) {
        arr.push((
          <div
            key={i}
            data-note-index={i}
            style={{
              left: (time || 0) * noteWidth,
              height: ((data! >> 16) & 0xff) / 1.27 + '%'
            }}
          />
        ))
      }
    })
    return arr
  }, [midi, noteWidth])

  if (!midi) return <div />

  return (
    <Box
      className='velocity'
      ref={ref}
      sx={{ '& div': { backgroundColor: color } }}
      onMouseDown={e => {
        if (e.target === e.currentTarget) return
        e.stopPropagation()
        if (hasSelected && !(e.target as any).className) return
        startY = e.pageY
        targetElm = e.target as any
        wrapperHeight = e.currentTarget.getBoundingClientRect().height
      }}
    >
      {notes}
    </Box>
  )
})

const EventEditor = memo(function EventEditor ({
  midi,
  color,
  beatWidth,
  noteWidth,
  contentWidth,
  eventEditorRef,
  timeSigNumerator,
  timeSigDenominator
}: {
  midi: packets.IMidiMessage[] | null | undefined
  color: string
  beatWidth: number
  noteWidth: number
  contentWidth: number
  eventEditorRef: React.Ref<HTMLDivElement>
  timeSigNumerator: number
  timeSigDenominator: number
}) {
  const theme = useTheme()
  const [selectedIndexes, setSelectedIndexes] = useState<Record<number, true | undefined>>({})
  useEffect(() => {
    $client.on('editor:selectedNotes', setSelectedIndexes)
    return () => {
      $client.off('editor:selectedNotes', setSelectedIndexes)
    }
  }, [])

  // 加载条状态
  const [loadingCirOn, setLoadingCir] = useState(false)
  const loadingDisplay = loadingCirOn ? 'block' : 'none'
  return (
    <Resizable enable={{ top: true }} minHeight={0} className='event-editor'>
      <Box className='event-editor-cir' sx={{ display: loadingDisplay }}><Box><CircularProgress /></Box></Box>
      <Paper
        square
        elevation={3}
        sx={{
          zIndex: 2,
          borderTop: '1px solid ' + theme.palette.primary.main,
          backgroundColor: theme.palette.background.bright
        }}
      >
        <Paper
          square
          elevation={6}
          className='event-editor-actions'
          sx={{
            flexDirection: 'column',
            overflow: 'hidden'
          }}
        >
          <Select value='velocity' variant='standard' className='editor-selector'>
            <MenuItem value='velocity'>力度</MenuItem>
          </Select>
          <Button
            variant='text'
            size='small'
            onClick={() => {
              if (!_midi) return
              const midis: IMidiMessages = {
                uuid: $globalData.activeTrack,
                midi: [],
                data: []
              }
              let targetMidiIndexes
              if (Object.keys(selectedIndexes).length > 0) {
                targetMidiIndexes = selectedIndexes
              } else {
                targetMidiIndexes = _midi
              }
              const waitSometime = Object.keys(targetMidiIndexes).length * (100 + Math.random() * 20 - Math.random() * 20)
              setLoadingCir(true)

              for (const id in targetMidiIndexes) {
                const randomVel: number = Math.floor((Math.random() * 50) + 60)
                const note = _midi[id]
                midis.data?.push(Number(id))
                const data = (note.data! << 16 >> 16) | (randomVel << 16)
                midis.midi?.push({
                  time: note.time,
                  data: data
                })
              }
              setTimeout(() => {
                $client.rpc.editMidiMessages(midis)
                setLoadingCir(false)
              }, waitSometime)
            }}
          >
            AI自动化
          </Button>
        </Paper>
        <Box
          className='content'
          ref={eventEditorRef}
          sx={{
            backgroundColor: theme.palette.mode === 'dark' ? alpha(theme.palette.primary.main, 0.1) : theme.palette.background.default,
            width: 0
          }}
        >
          <EventEditorGrid width={beatWidth} timeSigNumerator={timeSigNumerator}
            timeSigDenominator={timeSigDenominator}
          />
          <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
            <rect fill='url(#event-editor-grid-x)' x='0' y='0' width='100%' height='100%' />
          </svg>
          <div style={{
            width: contentWidth,
            height: '100%'
          }}
          >
            <Velocity midi={midi} color={color} noteWidth={noteWidth} />
          </div>
        </Box>
      </Paper>
    </Resizable>
  )
})

export default EventEditor
