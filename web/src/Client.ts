import ByteBuffer from 'bytebuffer'
import { colors } from '@mui/material'

export interface TrackInfo {
  uuid: string
  name: string
  muted: boolean
  solo: boolean
  color: string
  volume: number
}

const colorValues: string[] = []
for (const name in colors) colorValues.push((colors as any)[name][400])

export enum ServerboundPacket {
  Reply,
  SetProjectStatus,
  GetExplorerData,
  CreateTrack,
  Refresh,
  MidiMessage
}

export enum ClientboundPacket {
  Reply,
  ProjectStatus,
  SyncTrackInfo
}

export enum ExplorerType {
  Favorites,
  VSTPlugins
}

export default class Client {
  private ws: WebSocket
  private littleEndian = true
  private replyId = 0
  private events: Record<number, (buf: ByteBuffer) => void> = { }
  private replies: Record<number, (buf: ByteBuffer) => void> = { }
  public tracks: TrackInfo[] = []

  constructor (address: string, callback: () => void) {
    this.ws = new WebSocket(address)
    this.ws.onopen = () => {
      this.ws.binaryType = 'arraybuffer'
    }
    this.ws.onmessage = e => {
      const buf = ByteBuffer.wrap(e.data, this.littleEndian)
      const event = buf.readUint8()
      switch (event) {
        case ClientboundPacket.Reply: {
          const id = buf.readUint32()
          if (this.replies[id]) {
            this.replies[id](buf)
            delete this.replies[id]
          }
          break
        }
        case ClientboundPacket.ProjectStatus:
          this.littleEndian = buf.readUint8() === 1 && !buf.readUint8()
          if (callback) {
            callback()
            callback = null as any
          }
        // eslint-disable-next-line no-fallthrough
        default: this.events[event]?.(buf)
      }
    }
  }

  public on (id: ClientboundPacket, cb: (buf: ByteBuffer) => void) { this.events[id] = cb }
  public off (id: ClientboundPacket) { delete this.events[id] }

  public send (buf: ByteBuffer) { this.ws.send(buf.flip().toArrayBuffer()) }
  private buildPack (id: ServerboundPacket) { return new ByteBuffer(undefined, this.littleEndian, true).writeUint8(id) }
  private buildNeedReplyPack (id: ServerboundPacket) {
    const pid = this.replyId++
    return [new ByteBuffer(undefined, this.littleEndian, true).writeUint8(id).writeUint32(pid), new Promise<ByteBuffer>(resolve => (this.replies[pid] = resolve))] as const
  }

  public async getExplorerData (type: ExplorerType, path: string) {
    const [packet, promise] = this.buildNeedReplyPack(ServerboundPacket.GetExplorerData)
    this.send(packet.writeUint8(type).writeIString(path))
    const buf = await promise
    let len = buf.readUint32()
    const dirs = []
    const result: Record<string, boolean> = { }
    while (len-- > 0) dirs.push(buf.readIString())
    dirs.sort().forEach(it => (result[it] = true))
    const files = []
    len = buf.readUint32()
    while (len-- > 0) files.push(buf.readIString())
    files.sort().forEach(it => (result[it] = false))
    return result
  }

  public async createTrack (pos: number, identifier: string, name = '轨道' + (this.tracks.length + 1), color = colorValues[Math.random() * colorValues.length | 0]) {
    const [packet, promise] = this.buildNeedReplyPack(ServerboundPacket.CreateTrack)
    this.send(packet.writeIString(name).writeIString(color).writeUint8(pos).writeIString(identifier))
    const err = (await promise).readIString()
    if (err) throw new Error(err)
  }

  public refresh () { this.send(this.buildPack(ServerboundPacket.Refresh)) }

  public midiMessage (trackId: number, byte1: number, byte2: number, byte3: number) {
    this.send(this.buildPack(ServerboundPacket.MidiMessage).writeUint8(trackId).writeUint8(byte1).writeUint8(byte2).writeUint8(byte3))
  }

  public setProjectStatus (bpm: number, time: number, isPlaying: boolean, timeSigNumerator: number, timeSigDenominator: number) {
    this.send(this.buildPack(ServerboundPacket.SetProjectStatus).writeDouble(bpm).writeDouble(time).writeUint8(+isPlaying)
      .writeUint8(timeSigNumerator).writeUint8(timeSigDenominator).writeUint16(96000))
  }
}
