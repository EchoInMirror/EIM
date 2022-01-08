import './AppBar.less'
import React, { useState, useEffect } from 'react'
import { AppBar as MuiAppBar, Toolbar } from '@mui/material'
import { PlayArrow, Stop } from '@mui/icons-material'
import { ClientboundPacket } from '../Client'

const LeftSection: React.FC = () => {
  return (
    <section className='left-section'>
      <svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 964.59 907.06' width='46' height='46'>
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
    </section>
  )
}

const CenterSection: React.FC = () => {
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

const AppBar: React.FC = () => {
  const [bpm, setBPM] = useState(120)
  useEffect(() => {
    $client.on(ClientboundPacket.ProjectStatus, buf => {
      setBPM(buf.readDouble())
      console.log(buf.readDouble(), !!buf.readUint8(), buf.readUint8(), buf.readUint8(), buf.readUint16())
    })
    return () => $client.off(ClientboundPacket.ProjectStatus)
  }, [])
  return (
    <MuiAppBar position='fixed' className='app-bar'>
      <Toolbar>
        <LeftSection />
        <CenterSection />
        <section className='right-section'>
          <div className='info-block'>
            <div className='bpm'>
              <span className='integer'>{bpm | 0}</span>
              <span className='decimal'>{(bpm - bpm | 0).toFixed(2).slice(2)}</span>
            </div>
            <sub>BPM</sub>
          </div>
        </section>
      </Toolbar>
    </MuiAppBar>
  )
}

export default AppBar
