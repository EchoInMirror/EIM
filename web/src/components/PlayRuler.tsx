import React, { useEffect, useRef } from 'react'
import PlayArrowRounded from '@mui/icons-material/PlayArrowRounded'
import useGlobalData from '../reducer'
import { Paper, Box } from '@mui/material'

let movingPos = 0
let curElement: HTMLDivElement | undefined

const map: Record<string, React.RefObject<HTMLElement | null>> = { }
const map2: Record<string, React.RefObject<HTMLElement | null>> = { }
const map3: Record<string, React.RefObject<HTMLElement | null>> = { }
const map4: Record<string, HTMLElement> = { }

const barNumbers: JSX.Element[] = []
for (let i = 1; i < 1000; i++) barNumbers.push(<span key={i}>{i}</span>)

const calc = (target: HTMLElement, moveDelta: number) => {
  let val = (parseFloat(target.style.transform.slice(11)) || 0) + moveDelta
  const max = target.parentElement!.clientWidth - target.clientWidth
  val = Math.max(Math.min(val, max), 0)
  target.style.transform = `translateX(${val}px)`
  const id = target.dataset.scrollbarId as any
  const movable = map[id]?.current as HTMLElement
  if (!movable) return
  const val2 = movable.scrollLeft = val / max * (movable.scrollWidth - movable.clientWidth)
  const movable2 = map2[id]?.current as HTMLElement
  if (movable2) movable2.scrollLeft = val2
  const movable3 = map3[id]?.current as HTMLElement
  if (movable3) movable3.style.left = `-${val2}px`
}

export const moveScrollbar = (id: string, moveDelta: number) => {
  if (map4[id]) calc(map4[id], moveDelta)
}

const mousemove = (e: MouseEvent) => {
  if (!curElement) return
  calc(curElement, e.pageX - movingPos)
  movingPos = e.pageX
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

const observer = new ResizeObserver(elm => elm.forEach(it => calc(it.target as HTMLElement, 0)))

const PlayRuler: React.FC<{
  id: string
  headRef: React.Ref<HTMLDivElement>
  noteWidth: number
  movableRef: React.RefObject<HTMLElement | null>
  onWidthLevelChange: (value: boolean) => void
}> = ({ id, headRef, noteWidth, movableRef, onWidthLevelChange }) => {
  const [state] = useGlobalData()
  const ref = useRef<HTMLDivElement | null>(null)
  const ref2 = useRef<HTMLDivElement | null>(null)
  const ref3 = useRef<HTMLDivElement | null>(null)
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

  useEffect(() => {
    map[id] = movableRef
    map2[id] = ref2
    map3[id] = headRef as any
    return () => {
      delete map[id]
      delete map2[id]
      delete map3[id]
    }
  }, [movableRef])

  useEffect(() => {
    if (!ref3.current) return
    map4[id] = ref3.current
    observer.observe(ref3.current)
    return () => {
      if (ref3.current) observer.unobserve(ref3.current)
      delete map4[id]
    }
  }, [ref3.current])

  return (
    <Paper square elevation={3} className='play-ruler' sx={{ background: theme => theme.palette.background.bright }}>
      <Box className='scrollbar' ref={ref}>
        <div className='scrollbar-thumb' data-scrollbar-id={id} ref={ref3} />
      </Box>
      <Box
        className='bar-numbers'
        ref={ref2}
        sx={{ '& > span': { width: noteWidth * state.ppq * 4 } }}
        onClick={e => {
          const rect = e.currentTarget!.firstElementChild!.getBoundingClientRect()
          $client.setProjectStatus(0, Math.max(e.pageX - rect.left, 0) / noteWidth / state.ppq / state.bpm * 60, state.isPlaying, 0, 0)
        }}
        onWheel={e => onWidthLevelChange(e.deltaY > 0)}
      >
        {barNumbers}
      </Box>
      <div className='play-head' ref={headRef}><PlayArrowRounded /></div>
    </Paper>
  )
}

export default PlayRuler
