#include "WebSocketSession.h"
#include <boost/asio/post.hpp>
#include <boost/asio/dispatch.hpp>
#include "../Main.h"

WebSocketSession::~WebSocketSession() { state->leave(this); }

template<class Body, class Allocator> void WebSocketSession::run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req) {
    ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

    ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
        res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-chat-multi");
    }));

    ws.async_accept(req, boost::beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this()));
}

void WebSocketSession::doWrite() {
    ws.binary(true);
    ws.async_write(queue.front()->data(), boost::beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
}
void WebSocketSession::doRead() { ws.async_read(buffer, boost::beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this())); }
void WebSocketSession::onAccept(boost::beast::error_code ec) {
    if (ec) return;
    state->join(this);
    auto buf = boost::make_shared<ByteBuffer>();
    buf->writeInt8(0);
    buf->writeInt16(1);
    send(buf);
}
void WebSocketSession::onRead(boost::beast::error_code ec, std::size_t) {
    if (ec) return;

    int note;
    switch (buffer.readInt8()) {
    case 0:
        note = buffer.readInt8();
        ((GuiAppApplication*)juce::JUCEApplication::getInstance())->mainWindow->masterTrack->noteOn(note, buffer.readInt8());
        break;
    case 1:
        note = buffer.readInt8();
        ((GuiAppApplication*)juce::JUCEApplication::getInstance())->mainWindow->masterTrack->noteOff(note);
        break;
    }
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
