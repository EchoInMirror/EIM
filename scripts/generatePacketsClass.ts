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
for (const [, method, argType, returnType] of serverService.matchAll(/rpc (\w+) \((\w+)\) returns \((\w+|google\.protobuf\.Empty)\);/g)) {
  const name = method[0].toUpperCase() + method.slice(1)
  const argName = 'eim::' + argType
  const noArg = argType === EMPTY
  const noReturn = returnType === EMPTY
  serverServiceHandlers += `        case ${++id}: {
            ${noReturn
              ? ''
              : `unsigned int replyId = *(boost::asio::buffer_cast<unsigned int*>(buf.data()));
            buf.consume(4);`}
            ${noArg ? '' : argName + ' a;\n            a.ParseFromArray(ptr, len - 1);'}
            handle${name}(${noArg ? '' : 'a'}${noReturn
              ? ''
              : `${noArg ? '' : ', '}[replyId, session] (eim::${returnType} a) {
              auto replyBuf = std::make_shared<boost::beast::flat_buffer>();
              *(boost::asio::buffer_cast<unsigned char*>(replyBuf->prepare(1))) = 0;
              replyBuf->commit(1);
              *(boost::asio::buffer_cast<unsigned int*>(replyBuf->prepare(4))) = replyId;
              replyBuf->commit(4);
              auto len = a.ByteSizeLong();
              a.SerializePartialToArray(boost::asio::buffer_cast<void*>(replyBuf->prepare(len)), len);
              replyBuf->commit(len);
              session->send(replyBuf);
            }`});
            break;
        }\n`
  serverServiceClass += `\tvirtual void handle${name}(${noArg ? '' : argName + '&'}${noReturn ? '' : `${noArg ? '' : ', '}std::function<void (eim::${returnType})>`});\n`
  clientServiceServerPackets += `  ${name},\n`
  if (!noReturn) clientCallbacks += `  ${name}: true,\n`
}

const clientService = data.slice(data.indexOf('service ClientService {'))
id = 0
for (const [, method, argType, returnType] of clientService.matchAll(/rpc (\w+) \((\w+)\) returns \((\w+|google\.protobuf\.Empty)\);/g)) {
  const name = method[0].toUpperCase() + method.slice(1)
  const noArg = argType === EMPTY
  const methodDesc = `    std::shared_ptr<boost::beast::flat_buffer> make${name}Packet(${noArg ? '' : 'eim::' + argType + '& a'})`
  serverServiceHMethods += methodDesc + ';\n'
  serverServiceMethods += `${methodDesc} {
        auto buf = std::make_shared<boost::beast::flat_buffer>();
        *boost::asio::buffer_cast<unsigned char*>(buf->prepare(sizeof(unsigned char))) = ${++id};
        buf->commit(1);
        ${noArg
          ? ''
          : `        auto len = a.ByteSizeLong();
        a.SerializePartialToArray(boost::asio::buffer_cast<void*>(buf->prepare(len)), len);
        buf->commit(len);`}
        return buf;
    }
`

  clientServicePackets += `  ${name},\n`
  clientHandlerTypes += `  [ClientboundPacket.${name}]: (${noArg ? '' : 'data: eim.I' + argType}) ${returnType === EMPTY ? '=> void' : '=> Promise<eim.I' + returnType + '>'}\n`
  clientServiceHandlers += `      case ${id}:
        packet = ${noArg ? '' : 'eim.' + argType + '.decode(reader)'}
        break
`
}

fs.writeFileSync('packets/packets.h', `#pragma once
#include <boost/beast/core.hpp>
#include "packets.pb.h"

class WebSocketSession;

class ServerService {
public:
    void handlePacket(WebSocketSession* session, std::size_t len);
private:
${serverServiceClass.trimEnd()}
};

namespace EIMPackets {
${serverServiceHMethods.trimEnd()}
}
`)

fs.writeFileSync('packets/packets.cpp', `#include "packets.h"
#include "../../src/websocket/WebSocketSession.h"

void ServerService::handlePacket(WebSocketSession* session, std::size_t len) {
    auto& buf = session->buffer;
    unsigned char id = *(boost::asio::buffer_cast<unsigned char*>(buf.data()));
    buf.consume(1);
    void* ptr = boost::asio::buffer_cast<void*>(buf.data());
    switch (id) {
${serverServiceHandlers.trimEnd()}
    }
    buf.consume(len - 1);
};

namespace EIMPackets {
${serverServiceMethods.trimEnd()}
}
`)

fs.writeFileSync('packets/index.ts', `import { Reader } from 'protobufjs/minimal'
import { eim } from './packets'
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

// @ts-ignore
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

// @ts-ignore
interface ClientService {
  on <T extends number> (name: T, fn: HandlerTypes[T]): void
}

export default ClientService
`)
