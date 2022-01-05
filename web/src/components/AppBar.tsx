import './AppBar.less'
import React from 'react'
import { AppBar as MuiAppBar, Toolbar } from '@mui/material'
import { PlayArrow, Stop } from '@mui/icons-material'

const CenterSetion: React.FC = () => {
  return (
    <section className='center-section'>
      <div className='info-block'>
        <div className='time'>
          <span className='minute'>0</span>
          <span className='second'>00</span>
          <span className='microsecond'>000</span>
        </div>
        <sub>秒</sub>
      </div>
      <div className='info-block'>
        <div className='time'>
          <span className='minute'>0</span>
          <span className='second'>00</span>
          <span className='microsecond'>000</span>
        </div>
        <sub>小节</sub>
      </div>
      <PlayArrow fontSize='large' />
      <Stop fontSize='large' />
    </section>
  )
}

const RightSetion: React.FC = () => {
  return (
    <section className='right-section'>
      <div className='info-block'>
        <div className='bpm'>
          <span className='integer'>140</span>
          <span className='decimal'>00</span>
        </div>
        <sub>BPM</sub>
      </div>
    </section>
  )
}

const AppBar: React.FC = () => {
  return (
    <MuiAppBar position='fixed' sx={{ background: theme => theme.palette.background.lighter }} className='app-bar'>
      <Toolbar>
        <CenterSetion />
        <RightSetion />
      </Toolbar>
    </MuiAppBar>
  )
}

export default AppBar
