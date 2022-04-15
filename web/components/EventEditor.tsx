import './EventEditor.less'
import packets from '../../packets'
import React, { useMemo, memo } from 'react'
import { Resizable } from 're-resizable'
import { Paper, Box, useTheme, alpha } from '@mui/material'

const EventEditorGrid = memo(function EventEditorGrid ({ width, timeSigNumerator, timeSigDenominator }:
  { width: number, timeSigNumerator: number, timeSigDenominator: number }) {
  const theme = useTheme()
  const rectsX = []
  const gridXDeepColor = alpha(theme.palette.divider, 0.26)
  const beats = 16 / timeSigDenominator
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
        <pattern id='event-editor-grid-x' x='0' y='0' width={width * beats * timeSigNumerator} height='3240' patternUnits='userSpaceOnUse'>
          <g fill={theme.palette.divider}>{rectsX}</g>
        </pattern>
      </defs>
    </svg>
  )
})

const Velocity = memo(function Velocity ({ midi, color, noteWidth }: {
  midi: packets.IMidiMessage[] | null | undefined
  color: string
  noteWidth: number
}) {
  const notes = useMemo(() => {
    if (!midi) return
    const arr: JSX.Element[] = []
    midi.forEach(({ time, data }, i) => {
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
    <Box className='velocity' sx={{ '& div': { backgroundColor: color } }}>
      {notes}
    </Box>
  )
})

const EventEditor = memo(function EventEditor ({ midi, color, beatWidth, noteWidth, contentWidth, eventEditorRef, timeSigNumerator, timeSigDenominator }: {
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
  return (
    <Resizable enable={{ top: true }} minHeight={0} className='event-editor'>
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
        >
          力度
        </Paper>
        <Box
          className='content'
          ref={eventEditorRef}
          sx={{ backgroundColor: theme.palette.mode === 'dark' ? alpha(theme.palette.primary.main, 0.1) : theme.palette.background.default, width: 0 }}
        >
          <EventEditorGrid width={beatWidth} timeSigNumerator={timeSigNumerator} timeSigDenominator={timeSigDenominator} />
          <svg xmlns='http://www.w3.org/2000/svg' height='100%' className='grid'>
            <rect fill='url(#event-editor-grid-x)' x='0' y='0' width='100%' height='100%' />
          </svg>
          <div style={{ width: contentWidth, height: '100%' }}>
            <Velocity midi={midi} color={color} noteWidth={noteWidth} />
          </div>
        </Box>
      </Paper>
    </Resizable>
  )
})

export default EventEditor
