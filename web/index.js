import * as ByteBuffer from 'bytebuffer'

const buildPack = (id) => new ByteBuffer().writeByte(id)

const ws = new WebSocket('ws://127.0.0.1:8088')

const handlers = [
  data => {
    ByteBuffer.DEFAULT_ENDIAN = data.readByte() === 1 && !data.readByte() ? ByteBuffer.LITTLE_ENDIAN : ByteBuffer.BIG_ENDIAN
  }
]

ws.onopen = () => (ws.binaryType = 'arraybuffer')
document.getElementById('btn').onmousedown = () => ws.send(buildPack(0).writeByte(60).writeByte(80).flip().toArrayBuffer())
document.getElementById('btn').onmouseup = () => ws.send(buildPack(1).writeByte(60).flip().toArrayBuffer())
ws.onmessage = e => {
  const buf = ByteBuffer.wrap(e.data) 
  handlers[buf.readByte()](buf)
}
