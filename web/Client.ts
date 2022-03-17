import ClientService, { ServerboundPacket, callbacks } from '../packets'
import { colorValues } from './utils'
import { ReducerTypes, TrackInfo, TrackMidiNoteData } from './reducer'
import { eim } from '../packets/packets'

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
  private replies: Record<number, ((error: (Error | null), response?: (Uint8Array | null)) => void) | undefined> = { }

  public rpc: eim.ServerService

  constructor (address: string) {
    super()
    this.ws = new WebSocket(address)
    this.ws.onopen = () => {
      this.ws.binaryType = 'arraybuffer'
    }
    this.ws.onmessage = e => {
      const event = e.data[0]
      if (event === 0) {
        const id = new DataView(event).getUint32(1)
        const fn = this.replies[id]
        if (fn) {
          delete this.replies[id]
          fn(null, new Uint8Array(e.data, 5))
        }
      } else this.handlePacket(e.data)
    }
    this.rpc = eim.ServerService.create((method, data, callback) => {
      const name = method.name[0].toUpperCase() + method.name.slice(1)
      const id = serverboundPacketsMap[name]
      if (id == null) throw new Error('No such id: ' + name)
      const out = new Uint8Array(data.length + (callbacks[name] ? 5 : 1))
      out[0] = id
      if (callbacks[name]) {
        this.replies[this.replyId] = callback
        new DataView(out).setUint32(1, this.replyId++)
      } else callback(null, new Uint8Array())
      out.set(data, 1)
      this.ws.send(out)
    })
  }
}
