import React from 'react'
import PlayArrowRounded from '@mui/icons-material/PlayArrowRounded'
import { Paper } from '@mui/material'

const PlayRuler: React.FC<{ headRef: React.Ref<HTMLDivElement>, width: number }> = ({ headRef, width }) => {
  return (
    <Paper square elevation={3} className='play-ruler' sx={{ width }}>
      <div className='play-head' ref={headRef}>
        <PlayArrowRounded />
      </div>
    </Paper>
  )
}

export default PlayRuler
