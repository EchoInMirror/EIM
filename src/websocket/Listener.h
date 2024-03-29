#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "HttpSession.h"
#include "packets.pb.h"

class Listener : public boost::enable_shared_from_this<Listener> {
public:
    const std::shared_ptr<SharedState> state;
    Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint);

    void doAccept();
	void boardcastNotice(std::string message, EIMPackets::ClientboundSendMessage::MessageType type =
		EIMPackets::ClientboundSendMessage::MessageType::ClientboundSendMessage_MessageType_INFO);
    void boardcast(std::shared_ptr<boost::beast::flat_buffer> message);
    void boardcastExclude(std::shared_ptr<boost::beast::flat_buffer> message, WebSocketSession* session);
private:
    boost::asio::io_context& ioc;
    boost::asio::ip::tcp::acceptor acceptor;

    void onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);
};
