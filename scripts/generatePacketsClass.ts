import fs from 'fs'

const EMPTY = 'google.protobuf.Empty'
const data = fs.readFileSync('packets/packets.proto', 'utf8')

let serverServiceClass = ''
let serverServiceHMethods = ''
let serverServiceHandlers = ''
let serverServiceMethods = ''
let clientServiceHandlers = ''
let clientServicePackets = ''
let clientServiceServerPackets = ''
let clientHandlerTypes = ''
let clientCallbacks = ''

let serverService = data.slice(data.indexOf('service ServerService {'))
serverService = serverService.slice(0, serverService.indexOf('}'))
let id = 0
for (const [, method, argType, returnType] of serverService.matchAll(/rpc (\w+) \((\w+|google\.protobuf\.Empty)\) returns \((\w+|google\.protobuf\.Empty)\);/g)) {
  const name = method[0].toUpperCase() + method.slice(1)
  const argName = 'EIMPackets::' + argType
  const noArg = argType === EMPTY
  const noReturn = returnType === EMPTY
  serverServiceHandlers += `        case ${++id}: {
            ${noReturn
              ? ''
              : `unsigned int replyId = *(boost::asio::buffer_cast<unsigned int*>(buf.data()));
            buf.consume(4);`}
            ${noArg
              ? ''
              : `auto a = std::make_unique<${argName}>();
            a->ParseFromArray(boost::asio::buffer_cast<void*>(buf.data()), (int)len - ${noReturn ? 1 : 5});`}
            handle${name}(session${noArg ? '' : ', std::move(a)'}${noReturn
                ? ''
                : `${noArg && noReturn ? '' : ', '}[replyId, session] (EIMPackets::${returnType}& a) {
                auto replyBuf = std::make_unique<boost::beast::flat_buffer>();
                *(boost::asio::buffer_cast<unsigned char*>(replyBuf->prepare(1))) = 0;
                replyBuf->commit(1);
                *(boost::asio::buffer_cast<unsigned int*>(replyBuf->prepare(4))) = replyId;
                replyBuf->commit(4);
                auto len = a.ByteSizeLong();
                a.SerializePartialToArray(boost::asio::buffer_cast<void*>(replyBuf->prepare(len)), (int)len);
                replyBuf->commit(len);
                session->send(std::move(replyBuf));
            }`});
            break;
        }\n`
  serverServiceClass += `    void handle${name}(WebSocketSession*${noArg ? '' : `, std::unique_ptr<${argName}>`}${noReturn ? '' : `${noArg && noReturn ? '' : ', '}std::function<void (EIMPackets::${returnType}&)>`});\n`
  clientServiceServerPackets += `  ${name},\n`
  if (!noReturn) clientCallbacks += `  ${name}: true,\n`
}

const clientService = data.slice(data.indexOf('service ClientService {'))
id = 0
for (const [, method, argType, returnType] of clientService.matchAll(/rpc (\w+) \((\w+|google\.protobuf\.Empty)\) returns \((\w+|google\.protobuf\.Empty)\);/g)) {
  const name = method[0].toUpperCase() + method.slice(1)
  const noArg = argType === EMPTY
  const methodDesc = `    std::unique_ptr<boost::beast::flat_buffer> make${name}Packet(${noArg ? '' : 'EIMPackets::' + argType + '& a'})`
  serverServiceHMethods += methodDesc + ';\n'
  serverServiceMethods += `${methodDesc} {
        auto buf = std::make_unique<boost::beast::flat_buffer>();
        *boost::asio::buffer_cast<unsigned char*>(buf->prepare(sizeof(unsigned char))) = ${++id};
        buf->commit(1);
        ${noArg
          ? ''
          : `auto len = a.ByteSizeLong();
        a.SerializePartialToArray(boost::asio::buffer_cast<void*>(buf->prepare(len)), (int)len);
        buf->commit(len);`}
        return buf;
    }
`

  clientServicePackets += `  ${name},\n`
  clientHandlerTypes += `  [ClientboundPacket.${name}]: (${noArg ? '' : 'data: EIMPackets.I' + argType}) ${returnType === EMPTY ? '=> void' : '=> Promise<EIMPackets.I' + returnType + '>'}\n`
  clientServiceHandlers += `      case ${id}:
        packet = ${noArg ? '' : 'EIMPackets.' + argType + '.decode(reader)'}
        break
`
}

fs.writeFileSync('packets/packets.h', `#pragma once
#include <boost/beast/core.hpp>
#include "packets.pb.h"

class WebSocketSession;

namespace ServerService {
    void handlePacket(WebSocketSession* session, std::size_t len);
${serverServiceClass.trimEnd()}
};

namespace EIMMakePackets {
${serverServiceHMethods.trimEnd()}
}
`)

fs.writeFileSync('packets/packets.cpp', `#include "packets.h"
#include "../../src/websocket/WebSocketSession.h"

void ServerService::handlePacket(WebSocketSession* session, std::size_t len) {
    auto& buf = session->buffer;
    unsigned char id = *(boost::asio::buffer_cast<unsigned char*>(buf.data()));
    buf.consume(1);
    switch (id) {
${serverServiceHandlers.trimEnd()}
    }
    buf.consume(len - 1);
};

namespace EIMMakePackets {
${serverServiceMethods.trimEnd()}
}
`)

fs.writeFileSync('packets/index.ts', `import { Reader } from 'protobufjs/minimal'
import { EIMPackets } from './packets'
import { EventEmitter } from 'events'

export enum ClientboundPacket {
  Reply,
${clientServicePackets.slice(0, clientServicePackets.length - 2)}
}

export enum ServerboundPacket {
  Reply,
${clientServiceServerPackets.slice(0, clientServiceServerPackets.length - 2)}
}

export interface HandlerTypes {
${clientHandlerTypes.trimEnd()}
}

export const callbacks: Record<string, true> = {
${clientCallbacks.slice(0, clientCallbacks.length - 2)}
}

class ClientService extends EventEmitter {
  public handlePacket (data: ArrayBuffer) {
    const buf = new Uint8Array(data)
    const reader = new Reader(buf)
    reader.skip(1)
    let packet
    switch (buf[0]) {
${clientServiceHandlers.trimEnd()}
    }
    this.emit(buf[0] as any, packet)
  }
}

interface ClientService {
  on <T extends number> (name: T, fn: HandlerTypes[T]): this
  on (name: string | symbol, listener: (...args: any[]) => void): this
  once <T extends number> (name: T, fn: HandlerTypes[T]): this
  once (name: string | symbol, listener: (...args: any[]) => void): this
  off <T extends number> (name: T, fn: HandlerTypes[T]): this
  off (name: string | symbol, listener: (...args: any[]) => void): this
  emit <T extends number> (name: T, ...args: HandlerTypes[T] extends (...args: infer Args) => any ? Args : any[]): boolean
  emit (name: string | symbol, ...args: any[]): boolean
}

export default EIMPackets
export { ClientService }
`)
