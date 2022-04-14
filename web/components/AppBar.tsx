import './AppBar.less'
import React, { useEffect, useRef, useState, useMemo } from 'react'
import useGlobalData from '../reducer'
import Settings from './Settings'
import { AppBar as MuiAppBar, Toolbar, IconButton, TextField, Menu, MenuItem, Slider, ListItemIcon, ListItemText, Divider } from '@mui/material'
import { PlayArrow, Stop, Pause } from '@mui/icons-material'
import { useSnackbar } from 'notistack'
// import { playHeadRef as bottomBarPlayHeadRef, barLength as bottomBarLength } from './Editor'
import { playHeadRef as tracksPlayHeadRef, barLength as tracksLength } from './Tracks'
import { ClientboundPacket, HandlerTypes } from '../../packets'

import NoteAdd from '@mui/icons-material/NoteAdd'
import FileOpen from '@mui/icons-material/FileOpen'
import Save from '@mui/icons-material/Save'
import SaveAs from '@mui/icons-material/SaveAs'
import SaveAlt from '@mui/icons-material/SaveAlt'
import SettingsIcon from '@mui/icons-material/Settings'

const icon = (
  <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 964.59 907.06' width='40' height='40'>
    <defs>
      <clipPath id='clip-path' transform='translate(-15.78 -45.41)'>
        <path fill='none' transform='rotate(-50.26 768.342 413.676)' d='M529.38 17.84h477.94v791.67H529.38z' />
      </clipPath>
      <clipPath id='clip-path-2' transform='translate(-15.78 -45.41)'>
        <path transform='rotate(-50.26 231.226 586.306)' d='M-7.48 190.81h477.42v791H-7.48z' />
      </clipPath>
    </defs>
    <g fill='currentColor'>
      <g clipPath='url(#clip-path)'>
        <path d='M423.3 421.06L369 341.54a359.49 359.49 0 0 0-26 28.08l-.23.29 56.23 82.3a264.12 264.12 0 0 1 16.61-22.27q3.75-4.56 7.69-8.88zM395 318.68l53.39 78.23A259.37 259.37 0 0 1 479 374.73l-53.23-78A352.94 352.94 0 0 0 395 318.68zM455.72 279.7l53.81 78.85a261.91 261.91 0 0 1 207.31-2.83l62.34-75C677 228 556.17 228.86 455.72 279.7z' transform='translate(-15.78 -45.41)' />
        <path d='M835.25 454.16c61 93.47 57.13 219.43-18 309.75C725.06 874.84 560.37 890 449.44 797.78c-93.56-77.79-119-207.13-69.32-312.28L321.06 399c-100.45 149.48-73.42 353.73 68.07 471.36 151 125.54 375.17 104.9 500.7-46.1 108.63-130.66 107.8-316.13 7.76-445z' transform='translate(-15.78 -45.41)' />
      </g>
      <path d='M304.19 351.95L0 717.82l72.54 60.31L359.5 432.99l-55.31-81.04zM327.25 324.2l-.24.29 55.31 81.04 17.47-21.01-72.54-60.32z' />
      <g clipPath='url(#clip-path-2)'>
        <path d='M278.4 642.16l-62.34 75c102.05 52.66 222.67 51.9 323 1.29l-53.84-78.89a261.93 261.93 0 0 1-206.82 2.6zM515.83 623.44l53.23 78a353.71 353.71 0 0 0 30.69-21.83l-53.37-78.2a259.48 259.48 0 0 1-30.55 22.03zM579.68 568q-4 4.8-8.17 9.36l54.24 79.48a358.66 358.66 0 0 0 26.37-28.4L596 546.14A264.19 264.19 0 0 1 579.68 568z' transform='translate(-15.78 -45.41)' />
        <path d='M606.12 127.57C455.13 2 231 22.67 105.42 173.67-3.21 304.33-2.38 489.78 97.65 618.7l62.34-75C99 450.25 102.88 324.3 178 234c92.19-111 256.88-126.11 367.81-33.89 93.72 77.89 119.08 207.57 69.07 312.81l59 86.46c100.81-149.52 73.88-354.06-67.76-471.81z' transform='translate(-15.78 -45.41)' />
      </g>
      <path d='M564.79 521.46l68.33 56.8-52.1-76.33-16.23 19.53zM892.04 127.84L603.85 474.48l55.31 81.04 305.43-367.36-72.55-60.32z' />
      <circle cx='498' cy='498.22' r='50.44' transform='rotate(-50.26 441.702 492.331)' />
    </g>
  </svg>
)

// eslint-disable-next-line @typescript-eslint/no-unused-vars, @typescript-eslint/no-empty-function
let setProgress = (_val: number) => { }

let moving = false
const LeftSection: React.FC = () => {
  const [anchorEl, setAnchorEl] = React.useState<HTMLButtonElement>()
  const [progress, fn] = useState(0)
  const [open, setOpen] = useState(false)
  const [state] = useGlobalData()
  setProgress = fn

  return (
    <section className='left-section'>
      <IconButton color='inherit' onClick={e => setAnchorEl(e.target as HTMLButtonElement)} sx={{ margin: '0 -6px 0 -20px' }}>{icon}</IconButton>
      <Slider
        value={progress}
        onChange={(_, val) => {
          moving = true
          fn(val as number)
        }}
        max={state.maxNoteTime}
        step={1}
        onChangeCommitted={(_, position) => {
          moving = false
          $client.rpc.setProjectStatus({ position: position as number })
        }}
        sx={{ color: theme => theme.palette.mode === 'dark' ? '#fff' : 'rgba(0,0,0,0.87)' }}
      />
      <Menu anchorEl={anchorEl} open={!!anchorEl} onClose={() => setAnchorEl(undefined)}>
        <MenuItem onClick={() => setAnchorEl(undefined)}>
          <ListItemIcon><NoteAdd fontSize='small' /></ListItemIcon>
          <ListItemText>新建</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => setAnchorEl(undefined)}>
          <ListItemIcon><FileOpen fontSize='small' /></ListItemIcon>
          <ListItemText>打开</ListItemText>
        </MenuItem>
        <Divider />
        <MenuItem onClick={() => setAnchorEl(undefined)}>
          <ListItemIcon><Save fontSize='small' /></ListItemIcon>
          <ListItemText>保存</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => setAnchorEl(undefined)}>
          <ListItemIcon><SaveAs fontSize='small' /></ListItemIcon>
          <ListItemText>另存为</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => setAnchorEl(undefined)}>
          <ListItemIcon><SaveAlt fontSize='small' /></ListItemIcon>
          <ListItemText>导出...</ListItemText>
        </MenuItem>
        <Divider />
        <MenuItem
          onClick={() => {
            setOpen(true)
            setAnchorEl(undefined)
          }}
        >
          <ListItemIcon><SettingsIcon fontSize='small' /></ListItemIcon>
          <ListItemText>首选项</ListItemText>
        </MenuItem>
      </Menu>
      <Settings open={open} setOpen={setOpen} />
    </section>
  )
}

const CenterSection: React.FC = () => {
  const [state] = useGlobalData()
  const timeRef = useRef<HTMLSpanElement | null>(null)
  const barRef = useRef<HTMLSpanElement | null>(null)

  useEffect(() => {
    if (!timeRef.current || !barRef.current) return
    const minutesNode = timeRef.current as HTMLSpanElement
    const secondsNode = minutesNode.nextElementSibling as HTMLSpanElement
    const msNode = secondsNode.nextElementSibling as HTMLSpanElement
    const barsNode = barRef.current as HTMLSpanElement
    const beatsNode = barsNode.nextElementSibling as HTMLSpanElement
    const stepsNode = beatsNode.nextElementSibling as HTMLSpanElement
    const update = () => {
      const beats = $globalData.position / $globalData.ppq + ($globalData.isPlaying ? (Date.now() - $globalData.startTime) / 1000 / 60 * $globalData.bpm : 0)
      const time = beats / $globalData.bpm * 60
      minutesNode.innerText = (time / 60 | 0).toString().padStart(2, '0')
      secondsNode.innerText = (time % 60 | 0).toString().padStart(2, '0')
      msNode.innerText = (time - (time | 0)).toFixed(3).slice(2)
      barsNode.innerText = (1 + beats / $globalData.timeSigNumerator | 0).toString().padStart(2, '0')
      beatsNode.innerText = (1 + (beats | 0) % $globalData.timeSigNumerator).toString().padStart(2, '0')
      stepsNode.innerText = (1 + (beats - (beats | 0)) * (16 / $globalData.timeSigDenominator) | 0).toString()
      // if (bottomBarPlayHeadRef.current) bottomBarPlayHeadRef.current.style.transform = `translateX(${bottomBarLength * beats | 0}px)`
      if (tracksPlayHeadRef.current) tracksPlayHeadRef.current.style.transform = `translateX(${tracksLength * beats | 0}px)`
    }
    let cnt = 0
    const timer = setInterval(() => {
      if (!timeRef.current || !barRef.current) return
      // const curTime = state.currentTime + (Date.now() - state.startTime) / 1000
      update()
      if (!moving && ++cnt > 10) {
        setProgress($globalData.position + ($globalData.isPlaying ? (Date.now() - $globalData.startTime) / 1000 / 60 * $globalData.bpm * $globalData.ppq | 0 : 0))
        cnt = 0
      }
    }, 30)
    return () => clearInterval(timer)
  }, [timeRef.current, barRef.current])

  return (
    <section className='center-section'>
      <div className='info-block'>
        <div className='time'>
          <span className='minute' ref={timeRef} />
          <span className='second' />
          <span className='microsecond' />
        </div>
        <sub>秒</sub>
      </div>
      <div className='info-block'>
        <div className='time'>
          <span className='minute' ref={barRef} />
          <span className='second' />
          <span className='microsecond' />
        </div>
        <sub>小节</sub>
      </div>
      <IconButton
        color='inherit'
        onClick={() => $client.rpc.setProjectStatus({ isPlaying: !state.isPlaying })}
      >
        {state.isPlaying ? <Pause fontSize='large' /> : <PlayArrow fontSize='large' />}
      </IconButton>
      <IconButton
        color='inherit'
        onClick={() => $client.rpc.setProjectStatus({ isPlaying: false, position: 0 })}
      >
        <Stop fontSize='large' />
      </IconButton>
    </section>
  )
}

const AppBar: React.FC = () => {
  const [state] = useGlobalData()
  const [bpmInteger, setBPMInteger] = useState('120')
  const [bpmDecimal, setBPMDecimal] = useState('00')
  const [beatsAnchor, setBeatsAnchor] = useState<HTMLElement | undefined>()
  const [beatsAnchor2, setBeatsAnchor2] = useState<HTMLElement | undefined>()
  const { enqueueSnackbar } = useSnackbar()

  const updateBPM = (e: any) => {
    const bpm = +bpmInteger + (+bpmDecimal / 100)
    if (bpm === state.bpm) return
    e.target.blur()
    $client.rpc.setProjectStatus({ isPlaying: false, bpm })
    enqueueSnackbar('操作成功!', { variant: 'success' })
  }

  const beats = useMemo(() => {
    const arr: JSX.Element[] = []
    for (let i = 1; i <= 16; i++) {
      arr.push((
        <MenuItem
          key={i}
          onClick={() => {
            setBeatsAnchor(undefined)
            $client.rpc.setProjectStatus({ isPlaying: false, timeSigNumerator: i })
            enqueueSnackbar('操作成功!', { variant: 'success' })
          }}
        >
          {i}
        </MenuItem>
      ))
    }
    return arr
  }, [])

  const beats2 = useMemo(() => {
    const arr: JSX.Element[] = []
    for (let i = 1; i <= 16; i *= 2) {
      arr.push((
        <MenuItem
          key={i}
          onClick={() => {
            setBeatsAnchor(undefined)
            $client.rpc.setProjectStatus({ isPlaying: false, timeSigDenominator: i })
            enqueueSnackbar('操作成功!', { variant: 'success' })
          }}
        >
          {i}
        </MenuItem>
      ))
    }
    return arr
  }, [])

  useEffect(() => {
    const fn: HandlerTypes[ClientboundPacket.SetProjectStatus] = data => {
      if (data.bpm == null) return
      setBPMInteger((data.bpm | 0).toString())
      setBPMDecimal((data.bpm - data.bpm | 0).toFixed(2).slice(2))
    }
    $client.on(ClientboundPacket.SetProjectStatus, fn)
    return () => { $client.off(ClientboundPacket.SetProjectStatus, fn) }
  }, [])

  return (
    <MuiAppBar position='fixed' className='app-bar'>
      <Toolbar>
        <LeftSection />
        <CenterSection />
        <section className='right-section'>
          <div className='info-block'>
            <div className='beats'>
              <span onClick={(e: any) => setBeatsAnchor(e.target)}>{state.timeSigNumerator}</span>/
              <span onClick={(e: any) => setBeatsAnchor2(e.target)}>{state.timeSigDenominator}</span>
            </div>
            <Menu anchorEl={beatsAnchor} open={!!beatsAnchor} onClose={() => setBeatsAnchor(undefined)}>{beats}</Menu>
            <Menu anchorEl={beatsAnchor2} open={!!beatsAnchor2} onClose={() => setBeatsAnchor2(undefined)}>{beats2}</Menu>
            <sub>拍号</sub>
          </div>
          <div className='info-block'>
            <div className='bpm'>
              <TextField
                variant='standard'
                className='integer'
                onChange={e => {
                  const val = parseInt(e.target.value)
                  setBPMInteger((isNaN(val) ? 120 : Math.max(Math.min(val, 220), 10)).toString())
                }}
                onKeyDown={e => e.code === 'Enter' && updateBPM(e)}
                onBlur={updateBPM}
                value={bpmInteger}
              />.
              <TextField
                variant='standard'
                className='decimal'
                value={bpmDecimal}
                onBlur={updateBPM}
                onKeyDown={e => e.code === 'Enter' && updateBPM(e)}
                onChange={e => {
                  const val = parseInt(e.target.value)
                  setBPMDecimal((isNaN(val) ? 0 : Math.min(val, 99)).toString().padStart(2, '0'))
                }}
              />
            </div>
            <sub>BPM</sub>
          </div>
        </section>
      </Toolbar>
    </MuiAppBar>
  )
}

export default AppBar
