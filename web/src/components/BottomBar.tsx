import './BottomBar.less'
import React, { useContext, useEffect } from 'react'
import { Resizable } from 're-resizable'
import { Paper, Button, Box } from '@mui/material'
import { GlobalDataContext } from '../reducer'

const keyNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
const keys: JSX.Element[] = []
for (let i = 0; i < 132; i++) {
  keys.push(<Button key={i} data-eim-keyboard-key={i}>{keyNames[i % 12]} {i / 12 | 0}</Button>)
}

const Editor: React.FC = () => {
  const [state] = useContext(GlobalDataContext)
  useEffect(() => {
    let pressedKeys: number[] = []
    const mousedown = (e: MouseEvent) => {
      const btn = e.target as HTMLButtonElement
      const key = btn?.dataset?.eimKeyboardKey
      if (!key) return
      const index = $client.tracks.findIndex(it => it.uuid === state.activeTrack)
      if (!~index) return
      const rect = btn.getBoundingClientRect()
      const keyId = +key & 127
      pressedKeys.push(keyId)
      $client.midiMessage(index, 0x90, keyId, Math.min(rect.width, Math.max(0, e.pageX - rect.left)) / rect.width * 127 | 0)
    }
    const mouseup = () => {
      if (!pressedKeys.length) return
      const index = $client.tracks.findIndex(it => it.uuid === state.activeTrack)
      if (!~index) return
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
  return (
    <div className='editor'>
      <Box className='actions' sx={{ backgroundColor: theme => theme.palette.background.bright, width: 200, zIndex: 1 }}>
        当前轨道: {$client.tracks.find(it => it.uuid === state.activeTrack)?.name || '未选中'}
      </Box>
      <Paper square elevation={3} className='scrollable'>
        <Paper
          square
          elevation={5}
          className='keyboard'
          sx={{ '--black-note-color': (theme: any) => theme.palette.background.default } as any}
        >
          {keys}
        </Paper>
        <div className='notes'>23333333</div>
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
        sx={{ borderTop: theme => '1px solid ' + theme.palette.secondary.main, background: theme => theme.palette.background.bright, zIndex: 2 }}
        component='footer'
      >
        <Editor />
      </Paper>
    </Resizable>
  )
}

export default BottomBar
