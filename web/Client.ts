import packets, { ServerboundPacket, callbacks, ClientService, ClientboundPacket } from '../packets'
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
    this.setMaxListeners(0)
    this.ws = new WebSocket(address)
    this.ws.onopen = () => {
      this.ws.binaryType = 'arraybuffer'
      callback()
    }
    this.ws.onclose = e => this.emit('websocket:close', e.reason)
    this.ws.onmessage = e => {
      const view = new DataView(e.data)
      const event = view.getUint8(0)
      if (event === ClientboundPacket.Reply) {
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

    this.on(ClientboundPacket.SyncTracksInfo, data => {
      const tracks = data.isReplacing ? { } : { ...$globalData.tracks }
      data.tracks!.forEach(it => {
        if (it.isReplacing || !tracks[it.uuid!]) {
          tracks[it.uuid!] = it
          return
        }
        const obj: any = { ...tracks[it.uuid!] }
        for (const key in obj) {
          const val = (it as any)[key]
          if (val != null && !Array.isArray(val)) obj[key] = val
        }
        tracks[it.uuid!] = obj
      })
      $dispatch({ tracks })
    }).on(ClientboundPacket.RemoveTrack, data => {
      const old = $globalData.tracks
      const tracks: typeof old = { }
      for (const id in old) if (id !== data.value) tracks[id] = old[id]
      $dispatch({ tracks })
    }).on(ClientboundPacket.AddMidiMessages, data => {
      const track = $globalData.tracks[data.uuid!]
      if (!track) return
      data.midi!.forEach((it: any, i) => (it._oldIndex = i + track.midi!.length))
      track.midi = track.midi!.concat(data.midi!).sort((a, b) => (a.time || 0) - (b.time || 0))
      $dispatch({ })
    }).on(ClientboundPacket.DeleteMidiMessages, data => {
      const track = $globalData.tracks[data.uuid!]
      if (!track) return
      let index = 0
      track.midi = track.midi!.filter((it, i) => {
        if (data.data![index] !== i) {
          (it as any)._oldIndex = i
          return it
        }
        index++
      })
      $dispatch({ })
    }).on(ClientboundPacket.EditMidiMessages, data => {
      console.log(data)
      const track = $globalData.tracks[data.uuid!]
      if (!track) return
      track.midi = [...track.midi!]
      data.data!.forEach((index, i) => (track.midi![index] = data.midi![i]))
      track.midi!.forEach((it: any, i) => (it._oldIndex = i))
      track.midi!.sort((a, b) => (a.time || 0) - (b.time || 0))
      $dispatch({ })
    })
  }
}
