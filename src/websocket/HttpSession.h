#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/optional.hpp>
#include <cstdlib>
#include <memory>
#include "SharedState.h"

class HttpSession : public boost::enable_shared_from_this<HttpSession> {
private:
    boost::beast::tcp_stream stream;
    boost::beast::flat_buffer buffer;
    std::shared_ptr<SharedState> state;

    std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> parser;

    struct SendLambda;

    void onRead(boost::beast::error_code ec, std::size_t);
    void onWrite(boost::beast::error_code ec, std::size_t, bool close);
public:
    void doRead();
    HttpSession(boost::asio::ip::tcp::socket&& socket, std::shared_ptr<SharedState> const& state);
};
