#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "SharedState.h"
#include "ByteBuffer.h"

class WebSocketSession : public boost::enable_shared_from_this<WebSocketSession> {
public:
    explicit WebSocketSession(boost::asio::ip::tcp::socket&& socket, boost::shared_ptr<SharedState> const& state) : ws(std::move(socket)), state(state) {}
    ~WebSocketSession();

    void doWrite();
    void doRead();
    template<class Body, class Allocator> void run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req);
    void onAccept(boost::beast::error_code ec);
    void onRead(boost::beast::error_code ec, std::size_t);
    void onWrite(boost::beast::error_code ec, std::size_t);
    void onSend(boost::shared_ptr<ByteBuffer> ss);
    void send(boost::shared_ptr<ByteBuffer> ss);
private:
    ByteBuffer buffer;
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    boost::shared_ptr<SharedState> state;
    std::vector<boost::shared_ptr<ByteBuffer>> queue;
};
