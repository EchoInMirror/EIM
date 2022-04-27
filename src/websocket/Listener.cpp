#include "Listener.h"
#include "../Main.h"
#include "../packets.h"

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

	auto& label = EIMApplication::getEIMInstance()->mainWindow->statusLabel;
	label.setColour(juce::Label::ColourIds::textColourId, juce::Colours::green);
	label.setText("HTTP server created successfully!", {});
	EIMApplication::getEIMInstance()->mainWindow->textButton.setEnabled(false);
}

void Listener::doAccept() {
    acceptor.async_accept(boost::asio::make_strand(ioc), boost::beast::bind_front_handler(&Listener::onAccept, shared_from_this()));
}

void Listener::onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (ec) return;
    boost::make_shared<HttpSession>(std::move(socket), state)->doRead();
    doAccept();
}

void Listener::boardcast(std::shared_ptr<boost::beast::flat_buffer> message) { state->send(message); }
void Listener::boardcastExclude(std::shared_ptr<boost::beast::flat_buffer> message, WebSocketSession* session) { state->sendExclude(message, session); }
void Listener::boardcastNotice(std::string message, EIMPackets::ClientboundSendMessage::MessageType type) {
	EIMPackets::ClientboundSendMessage msg;
	msg.set_message(message);
	msg.set_type(type);
	boardcast(EIMMakePackets::makeSendMessagePacket(msg));
}
