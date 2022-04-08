import './BottomBar.less'
import React, { useState } from 'react'
import { Resizable } from 're-resizable'
import { Paper } from '@mui/material'

import Mixer from './Mixer'
import Editor from './Editor'

// eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-empty-function
export let setIsMixer = (_val: boolean) => { }
// {isMixer ? <Mixer /> : <Editor />}
const BottomBar: React.FC = () => {
  const [isMixer, fn] = useState(true)
  setIsMixer = fn
  return (
    <Resizable enable={{ top: true }} className='bottom-bar' maxHeight='80vh' minHeight={0}>
      <Paper
        square
        id='bottom-bar'
        elevation={3}
        sx={{ borderTop: theme => '1px solid ' + theme.palette.primary.main, background: theme => theme.palette.background.bright, zIndex: 2 }}
        component='footer'
      >
      </Paper>
    </Resizable>
  )
}

export default BottomBar
