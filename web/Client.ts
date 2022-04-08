import packets, { ServerboundPacket, callbacks, ClientService } from '../packets'
import { colorValues } from './utils'
import { ReducerTypes, TrackInfo, TrackMidiNoteData } from './reducer'
import type { RPCImplCallback } from 'protobufjs'

export enum ExplorerType {
  Favorites,
  VSTPlugins
}

export interface Config {
  vstSearchPaths: Record<string, string[]>
}

const serverboundPacketsMap: Record<string, number> = { }

for (const id in ServerboundPacket) serverboundPacketsMap[ServerboundPacket[id]] = +id

export default class Client extends ClientService {
  private ws: WebSocket
  private replyId = 0
  private replies: Record<number, RPCImplCallback | undefined> = { }

  public rpc: packets.ServerService

  constructor (address: string, callback: () => void) {
    super()
    this.ws = new WebSocket(address)
    this.ws.onopen = () => {
      this.ws.binaryType = 'arraybuffer'
      callback()
    }
    this.ws.onmessage = e => {
      const view = new DataView(e.data)
      const event = view.getUint8(0)
      if (event === 0) {
        const id = view.getUint32(1)
        const fn = this.replies[id]
        if (fn) {
          delete this.replies[id]
          fn(null, new Uint8Array(e.data, 5))
        }
      } else this.handlePacket(e.data)
    }
    this.rpc = packets.ServerService.create((method, data, callback) => {
      const name = method.name[0].toUpperCase() + method.name.slice(1)
      const id = serverboundPacketsMap[name]
      if (id == null) throw new Error('No such id: ' + name)
      const hasCallback = callbacks[name]
      const out = new Uint8Array(data.length + (hasCallback ? 5 : 1))
      out[0] = id
      if (hasCallback) {
        this.replies[this.replyId] = callback
        out[1] = (this.replyId >> 24) & 0xFF
        out[2] = (this.replyId >> 16) & 0xFF
        out[3] = (this.replyId >> 8) & 0xFF
        out[4] = this.replyId & 0xFF
        out.set(data, 5)
        this.replyId++
      } else {
        callback(null, new Uint8Array())
        out.set(data, 1)
      }
      this.ws.send(out)
    })
  }
}
