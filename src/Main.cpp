#include "Main.h"
#include <string>
#include "ByteBuffer.h"

class session : public std::enable_shared_from_this<session> {
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws;
    ByteBuffer buffer;

public:
    explicit session(boost::asio::ip::tcp::socket&& socket) : ws(std::move(socket)) {}

    void run() {
        boost::asio::dispatch(ws.get_executor(), boost::beast::bind_front_handler(&session::onRun, shared_from_this()));
    }

    void onRun() {
        ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
        ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::response_type& res) {
            res.set(boost::beast::http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) +
                " websocket-server-async");
            }));

        ws.async_accept(boost::beast::bind_front_handler(&session::onAccept, shared_from_this()));
    }

    void doWrite() {
        ws.binary(true);
        ws.async_write(buffer.data(), boost::beast::bind_front_handler(&session::onWrite, shared_from_this()));
    }

    void doRead() { ws.async_read(buffer, boost::beast::bind_front_handler(&session::onRead, shared_from_this())); }

    void onAccept(boost::beast::error_code ec) {
        if (ec) return;
        buffer.writeInt8(0);
        buffer.writeInt16(1);
        doWrite();
    }

    void onRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return;

        int note;
        switch (buffer.readInt8()) {
        case 0:
            note = buffer.readInt8();
            DBG("On: " << note);
            ((GuiAppApplication*)juce::JUCEApplication::getInstance())->masterTrack->noteOn(note, buffer.readInt8());
            break;
        case 1:
            note = buffer.readInt8();
            DBG("Off: " << note);
            ((GuiAppApplication*)juce::JUCEApplication::getInstance())->masterTrack->noteOff(note);
            break;
        }
        doRead();
    }

    void onWrite(boost::beast::error_code ec, std::size_t bytes_transferred){
        boost::ignore_unused(bytes_transferred);
        if (ec) return;
        buffer.consume(buffer.size());
        doRead();
    }
};

class listener : public std::enable_shared_from_this<listener> {
    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;

public:
    listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint): ioc_(ioc), acceptor_(ioc) {
        boost::beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) return;

        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec) return;

        acceptor_.bind(endpoint, ec);
        if (ec) return;

        acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec) return;
    }

    void run() { do_accept(); }

private:
    void do_accept() {
        acceptor_.async_accept(boost::asio::make_strand(ioc_), boost::beast::bind_front_handler(&listener::onAccept, shared_from_this()));
    }

    void onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (!ec) std::make_shared<session>(std::move(socket))->run();
        do_accept();
    }
};

void GuiAppApplication::initialise(const juce::String& commandLine) {
    juce::ignoreUnused(commandLine);
    masterTrack.reset(new MasterTrack());
    auto address = boost::asio::ip::make_address("0.0.0.0");

    std::make_shared<listener>(ioc, boost::asio::ip::tcp::endpoint{ address, 8088 })->run();

    v.reserve(4);
    for (auto i = 4; i > 0; --i) v.emplace_back([this] { ioc.run(); });
}

START_JUCE_APPLICATION(GuiAppApplication)
