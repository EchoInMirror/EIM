import React from 'react'
import ByteBuffer from 'bytebuffer'
import { render } from 'react-dom'

import App from './components/App'

// const buildPack = (id: number) => new ByteBuffer().writeByte(id)

const ws = new WebSocket('ws://127.0.0.1:8088')
let littleEndian = true

const handle = (data: ByteBuffer) => {
  switch (data.readByte()) {
    case 0:
      littleEndian = data.readByte() === 1 && !data.readByte()
  }
}

ws.onopen = () => (ws.binaryType = 'arraybuffer')
// document.getElementById('btn')!.onmousedown = () => ws.send(buildPack(0).writeByte(60).writeByte(80).flip().toArrayBuffer())
// document.getElementById('btn')!.onmouseup = () => ws.send(buildPack(1).writeByte(60).flip().toArrayBuffer())
ws.onmessage = e => {
  const buf = ByteBuffer.wrap(e.data, undefined, littleEndian)
  handle(buf)
}

render(<App />, document.getElementById('root'))
