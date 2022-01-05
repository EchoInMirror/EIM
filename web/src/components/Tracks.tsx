import './Tracks.less'
import React, { useRef, useEffect } from 'react'
import { Paper, Box, Toolbar, Button, useTheme, Slider, Stack } from '@mui/material'
import { VolumeUp } from '@mui/icons-material'

const Tracks: React.FC = () => {
  const theme = useTheme()
  const ref = useRef<HTMLDivElement | null>(null)
  useEffect(() => {
    if (!ref.current) return
    const fn = () => {
      const elm = document.getElementById('bottom-bar')
      if (!elm) return
      ref.current!.style.height = (window.innerHeight - ref.current!.offsetTop - elm.clientHeight) + 'px'
    }
    fn()
    document.addEventListener('resize', fn)
    return () => document.removeEventListener('resize', fn)
  }, [ref.current])
  return (
    <main className='tracks'>
      <Toolbar />
      <div ref={ref} className='wrapper'>
        <Paper square elevation={3} component='ol' sx={{ background: 'inherit', zIndex: 1 }}>
          <li>
            <div className='color' style={{ backgroundColor: '#50fa7b' }} />
            <div className='title'>
              <Button className='solo' variant='outlined' />
              <span className='name'>Pigments</span>
            </div>
            <div>
              <Stack spacing={1} direction='row' alignItems='center'>
                <VolumeUp fontSize='small' /><Slider size='small' valueLabelDisplay='auto' defaultValue={80} />
              </Stack>
            </div>
          </li>
        </Paper>
        <Box className='playlist' sx={{ backgroundColor: theme.palette.background.default }}>Play list</Box>
      </div>
    </main>
  )
}

export default Tracks
