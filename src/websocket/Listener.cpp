#include "Listener.h"
#include "Packets.h"
#include "../Main.h"

Listener::Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint) : ioc(ioc), acceptor(ioc), state(std::make_shared<SharedState>(".")) {
    boost::beast::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    if (ec) return;

    acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) return;

    acceptor.bind(endpoint, ec);
    if (ec) return;

    acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) return;
}

void Listener::doAccept() {
    acceptor.async_accept(boost::asio::make_strand(ioc), boost::beast::bind_front_handler(&Listener::onAccept, shared_from_this()));
}

void Listener::onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (ec) return;
    boost::make_shared<HttpSession>(std::move(socket), state)->doRead();
    doAccept();
}

void Listener::syncTrackInfo() {
    auto& tracks = EIMApplication::getEIMInstance()->mainWindow->masterTrack->tracks;
    auto buf = EIMPackets::makePacket(ClientboundPacket::ClientboundSyncTrackInfo);
    buf->writeInt8((char)tracks.size());
    for (auto& it : tracks) {
        auto track = (Track*)it->getProcessor();
        buf->writeUUID(track->uuid);
        track->writeTrackInfo(buf.get());
    }
    state->send(buf);
}

void Listener::broadcastProjectStatus() {
    state->send(EIMPackets::makeProjectStatusPacket());
}
