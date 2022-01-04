#include "Main.h"
#include <string>

class session : public std::enable_shared_from_this<session>
{
    boost::beast::websocket::stream<boost::beast::tcp_stream> ws_;
    boost::beast::flat_buffer buffer_;

public:
    explicit session(boost::asio::ip::tcp::socket&& socket) : ws_(std::move(socket)) {}

    void run() {
        boost::asio::dispatch(ws_.get_executor(), boost::beast::bind_front_handler(&session::on_run, shared_from_this()));
    }

    void on_run() {
        ws_.set_option(
            boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::response_type& res)
            {
                res.set(boost::beast::http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-server-async");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            boost::beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void on_accept(boost::beast::error_code ec) {
        if (!ec) do_read();
    }

    void do_read() {
        ws_.async_read(buffer_, boost::beast::bind_front_handler(&session::on_read, shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec == boost::beast::websocket::error::closed) return;

        ws_.text(ws_.got_text());
        ws_.async_write(buffer_.data(), boost::beast::bind_front_handler(&session::on_write, shared_from_this()));
    }

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred){
        boost::ignore_unused(bytes_transferred);
        if (ec) return;
        buffer_.consume(buffer_.size());
        do_read();
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
        acceptor_.async_accept(boost::asio::make_strand(ioc_), boost::beast::bind_front_handler(&listener::on_accept, shared_from_this()));
    }

    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
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
