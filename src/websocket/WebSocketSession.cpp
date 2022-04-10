#include "WebSocketSession.h"
#include <boost/asio/post.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/bind_executor.hpp>
#include "../Main.h"

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
    EIMPackets::ClientboundTracksInfo info;
    info.set_isreplacing(true);
    auto instance = EIMApplication::getEIMInstance();
    for (auto& track : instance->mainWindow->masterTrack->tracks) info.add_tracks()->CopyFrom(((Track*)track->getProcessor())->getTrackInfo());
    instance->listener->state->send(std::move(EIMMakePackets::makeSyncTracksInfoPacket(&info)));
    // send(EIMPackets::makeAllTrackMidiDataPacket());
    // if (EIMApplication::getEIMInstance()->pluginManager->isScanning) send(EIMPackets::makeScanVSTsPacket(true));
}
void WebSocketSession::onRead(boost::beast::error_code ec, std::size_t len) {
    if (ec) return;
    ServerService::handlePacket(this, len);
    doRead();
}
void WebSocketSession::onWrite(boost::beast::error_code ec, std::size_t) {
    if (ec) return;
    queue.erase(queue.begin());
    if (!queue.empty()) doWrite();
}
void WebSocketSession::send(std::shared_ptr<boost::beast::flat_buffer> data) {
    boost::asio::post(ws.get_executor(), boost::beast::bind_front_handler(&WebSocketSession::onSend, shared_from_this(), data));
}
void WebSocketSession::onSend(std::shared_ptr<boost::beast::flat_buffer> ss) {
    queue.push_back(ss);
    if (queue.size() > 1) return;
    doWrite();
}
