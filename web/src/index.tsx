import React from 'react'
import { render } from 'react-dom'

import App from './components/App'
import Client from './Client'

// const buildPack = (id: number) => new ByteBuffer().writeByte(id)

// document.getElementById('btn')!.onmousedown = () => ws.send(buildPack(0).writeByte(60).writeByte(80).flip().toArrayBuffer())
// document.getElementById('btn')!.onmouseup = () => ws.send(buildPack(1).writeByte(60).flip().toArrayBuffer())
window.$client = new Client('ws://127.0.0.1:8088', () => render(<App />, document.getElementById('root')))
