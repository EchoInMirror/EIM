import './BottomBar.less'
import React from 'react'
import { Paper } from '@mui/material'

const BottomBar: React.FC = () => {
  return (
    <Paper
      square
      id='bottom-bar'
      elevation={3}
      className='bottom-bar'
      sx={{ borderTop: theme => '1px solid ' + theme.palette.secondary.main, background: theme => theme.palette.background.bright, zIndex: 2 }}
      component='footer'
    >
      某个轨道的插件和效果器
    </Paper>
  )
}

export default BottomBar
