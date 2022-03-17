#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include "SharedState.h"
#include "../packets/packets.h"

class WebSocketSession : public boost::enable_shared_from_this<WebSocketSession>, private ServerService {
public:
    explicit WebSocketSession(boost::asio::ip::tcp::socket&& socket, std::shared_ptr<SharedState> const& state) : ws(std::move(socket)), state(state) {}
    ~WebSocketSession();
    boost::beast::flat_buffer buffer;
    std::shared_ptr<SharedState> state;

    void doWrite();
    void doRead();
    template<class Body, class Allocator> void run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req) {
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));

        ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
            res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-chat-multi");
        }));

        ws.async_accept(req, boost::beast::bind_front_handler(&WebSocketSession::onAccept, shared_from_this()));
    }
    void onAccept(boost::beast::error_code ec);
    void onRead(boost::beast::error_code ec, std::size_t);
    void onWrite(boost::beast::error_code ec, std::size_t);
    void onSend(std::shared_ptr<boost::beast::flat_buffer> ss);
    void send(std::shared_ptr<boost::beast::flat_buffer> ss);

    virtual void handleSetProjectStatus(std::unique_ptr<eim::ProjectStatus>) override;
    virtual void handleGetExplorerData(std::unique_ptr<eim::ServerboundExplorerData>, std::function<void(eim::ClientboundExplorerData)>) override;
private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    std::vector<std::shared_ptr<boost::beast::flat_buffer>> queue;
};
