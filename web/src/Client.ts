import ByteBuffer from 'bytebuffer'
import { colorValues } from './utils'
import { ReducerTypes, TrackInfo, TrackMidiNoteData } from './reducer'

export enum ServerboundPacket {
  Reply,
  SetProjectStatus,
  GetExplorerData,
  CreateTrack,
  Refresh,
  MidiMessage,
  UpdateTrackInfo,
  MidiNotesAdd,
  MidiNotesDelete,
  MidiNotesEdit,
  OpenPluginManager,
  Config
}

export enum ClientboundPacket {
  Reply,
  ProjectStatus,
  SyncTrackInfo,
  TrackMidiData,
  UpdateTrackInfo,
  Config
}

export enum ExplorerType {
  Favorites,
  VSTPlugins
}

export interface Config {
  vstSearchPaths: string[]
}

const readTrack = (buf: ByteBuffer, uuid = buf.readIString()): TrackInfo => ({
  uuid,
  name: buf.readIString(),
  color: buf.readIString(),
  volume: buf.readFloat(),
  muted: !!buf.readUint8(),
  solo: !!buf.readUint8()
})

export default class Client {
  private ws: WebSocket
  private littleEndian = true
  private replyId = 0
  private events: Record<number, (buf: ByteBuffer) => void> = { }
  private replies: Record<number, (buf: ByteBuffer) => void> = { }
  public trackNameToIndex: Record<string, number> = {}
  public trackUpdateNotifier: Record<string, () => void> = {}

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
          break
        case ClientboundPacket.TrackMidiData: {
          let len = buf.readUint8()
          const trackMidiData = { ...$globalData.trackMidiData }
          let maxNoteTime = 0
          while (len-- > 0) {
            const key = buf.readIString()
            trackMidiData[key] = { notes: [] }
            const { notes } = trackMidiData[key]
            let cnt = buf.readUint16()
            while (cnt-- > 0) {
              let startTime: number
              let endTime: number
              notes.push([buf.readUint8(), buf.readUint8(), startTime = buf.readUint32(), endTime = buf.readUint32()])
              const curTime = startTime + endTime
              if (curTime > maxNoteTime) maxNoteTime = curTime
            }
          }
          $dispatch({ type: ReducerTypes.SetTrackMidiData, trackMidiData, maxNoteTime })
          break
        }
        case ClientboundPacket.SyncTrackInfo: {
          let len = buf.readUint8()
          const tracks: TrackInfo[] = []
          this.trackNameToIndex = { }
          while (len-- > 0) {
            const it = readTrack(buf)
            this.trackNameToIndex[it.uuid] = tracks.push(it) - 1
          }
          $dispatch({ type: ReducerTypes.SetTrackInfo, tracks })
          break
        }
        case ClientboundPacket.UpdateTrackInfo: {
          const id = buf.readUint8()
          if (!$globalData.tracks[id]) return
          $globalData.tracks[id] = readTrack(buf, $globalData.tracks[id].uuid)
          $dispatch({ type: ReducerTypes.SetTrackInfo, tracks: [...$globalData.tracks] })
          break
        }
      }
      this.events[event]?.(buf)
    }
  }

  public on (id: ClientboundPacket, cb: (buf: ByteBuffer) => void) {
    this.events[id] = cb
    return this
  }

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

  public async createTrack (pos: number, identifier: string, name = '轨道', color = colorValues[Math.random() * colorValues.length | 0]) {
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

  public updateTrackInfo (id: number, name = '', color = '', volume = -1, muted = false, solo = false) {
    this.send(this.buildPack(ServerboundPacket.UpdateTrackInfo).writeUint8(id).writeIString(name).writeIString(color)
      .writeFloat(volume).writeUint8(+muted).writeUint8(+solo))
  }

  public addMidiNotes (id: number, notes: TrackMidiNoteData[]) {
    const buf = this.buildPack(ServerboundPacket.MidiNotesAdd).writeUint8(id).writeUint16(notes.length)
    notes.forEach(it => buf.writeUint8(it[0]).writeUint8(it[1]).writeUint32(it[2]).writeUint32(it[3]))
    this.send(buf)
  }

  public deleteMidiNotes (id: number, notes: TrackMidiNoteData[]) {
    const buf = this.buildPack(ServerboundPacket.MidiNotesDelete).writeUint8(id).writeUint16(notes.length)
    notes.forEach(it => buf.writeUint8(it[0]).writeUint32(it[2]))
    this.send(buf)
  }

  public editMidiNotes (id: number, notes: [number, number][], dx: number, dy: number, dw: number, dv: number) {
    if (!dx && !dy && !dw && !dv) return
    const buf = this.buildPack(ServerboundPacket.MidiNotesEdit).writeInt8(id).writeInt16(notes.length).writeInt32(dx).writeInt8(dy).writeInt32(dw).writeFloat(dv)
    notes.forEach(it => buf.writeUint8(it[0]).writeUint32(it[1]))
    this.send(buf)
  }

  public openPluginManager () {
    this.send(this.buildPack(ServerboundPacket.OpenPluginManager))
  }

  public config (config?: Config) {
    const [packet, promise] = this.buildNeedReplyPack(ServerboundPacket.Config)
    packet.writeUint8(config ? 1 : 0)
    if (config) packet.writeIString(JSON.stringify(config))
    this.send(packet)
    return promise.then(it => it.readIString())
  }
}
