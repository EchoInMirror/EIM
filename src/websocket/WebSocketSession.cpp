#include "WebSocketSession.h"
#include <boost/asio/post.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/bind_executor.hpp>
#include "../Main.h"
#include "Packets.h"

WebSocketSession::~WebSocketSession() { state->leave(this); }

void WebSocketSession::doWrite() {
    ws.binary(true);
    ws.async_write(queue.front()->data(), boost::beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
}
void WebSocketSession::doRead() { ws.async_read(buffer, boost::beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this())); }
void WebSocketSession::onAccept(boost::beast::error_code ec) {
    if (ec) return;
    state->join(this);
    doRead();
    send(makeProjectStatusPacket());
    send(makeAllTrackMidiDataPacket());
}
void WebSocketSession::onRead(boost::beast::error_code ec, std::size_t) {
    if (ec) return;
    EIMApplication::getEIMInstance()->handlePacket(this);
    doRead();
}
void WebSocketSession::onWrite(boost::beast::error_code ec, std::size_t) {
    if (ec) return;
    queue.erase(queue.begin());
    if (!queue.empty()) doWrite();
}
void WebSocketSession::send(boost::shared_ptr<ByteBuffer> data) {
    boost::asio::post(ws.get_executor(), boost::beast::bind_front_handler(&WebSocketSession::onSend, shared_from_this(), data));
}
void WebSocketSession::onSend(boost::shared_ptr<ByteBuffer> ss) {
    queue.push_back(ss);
    if (queue.size() > 1) return;
    doWrite();
}
