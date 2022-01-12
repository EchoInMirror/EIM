import React, { useEffect, useRef, useMemo } from 'react'
import PlayArrowRounded from '@mui/icons-material/PlayArrowRounded'
import useGlobalData from '../reducer'
import { Paper, Box } from '@mui/material'

let movingPos = 0
let curElement: HTMLDivElement | undefined

const map: Record<number, React.RefObject<HTMLElement | null>> = { }

const mousemove = (e: MouseEvent) => {
  if (!curElement) return
  const target = curElement as HTMLDivElement
  let val = (parseFloat(target.style.transform.slice(11)) || 0) + e.pageX - movingPos
  movingPos = e.pageX
  const max = target.parentElement!.clientWidth - target.clientWidth
  val = Math.max(Math.min(val, max), 0)
  target.style.transform = `translateX(${val}px)`
  const movable = map[target.dataset.scrollbarId as any]?.current as HTMLElement
  if (!movable) return
  movable.scrollLeft = val / max * (movable.scrollWidth - movable.clientWidth)
}
const mousedown = (e: any) => {
  if (e.target?.className !== 'scrollbar-thumb') return
  curElement = e.target
  movingPos = e.pageX
  document.addEventListener('mousemove', mousemove)
}
const mouseup = () => {
  curElement = undefined
  document.removeEventListener('mousemove', mousemove)
}
document.addEventListener('mouseup', mouseup)
document.addEventListener('mousedown', mousedown)

let id = 0
const PlayRuler: React.FC<{ headRef: React.Ref<HTMLDivElement>, noteWidth: number, movableRef: React.RefObject<HTMLElement | null> }> = ({ headRef, noteWidth, movableRef }) => {
  const [state] = useGlobalData()
  const ref = useRef<HTMLDivElement | null>(null)
  useEffect(() => {
    const elm = ref.current
    if (!elm) return
    const thumb = elm.firstElementChild! as HTMLDivElement
    thumb.ondrag = console.log
    const observer = new ResizeObserver(([{ contentRect }]) => {
      thumb.style.width = Math.min(Math.max(contentRect.width / (state.maxNoteTime + state.ppq * 4) / noteWidth, 0.1), 1) * 100 + '%'
    })
    observer.observe(elm)
    thumb.style.width = Math.min(Math.max(elm.clientWidth / (state.maxNoteTime + state.ppq * 4) / noteWidth, 0.1), 1) * 100 + '%'
    return () => observer.disconnect()
  }, [ref.current, state.maxNoteTime, noteWidth, state.ppq])
  const curId = useMemo(() => id++, [])
  useEffect(() => {
    map[curId] = movableRef
    return () => { delete map[curId] }
  }, [movableRef])

  return (
    <Paper square elevation={3} className='play-ruler'>
      <Box className='scrollbar' ref={ref} sx={{ '& div': { boxShadow: theme => theme.shadows[3] } }}>
        <div className='scrollbar-thumb' data-scrollbar-id={curId} />
      </Box>
      <div className='play-head' ref={headRef}>
        <PlayArrowRounded />
      </div>
    </Paper>
  )
}

export default PlayRuler
