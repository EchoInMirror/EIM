#include "HttpSession.h"
#include "WebSocketSession.h"
#include <iostream>
#include <boost/beast.hpp>
#include <boost/config.hpp>

boost::beast::string_view getMimeType(boost::beast::string_view path) {
    using boost::beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == boost::beast::string_view::npos) return boost::beast::string_view{};
        return path.substr(pos);
    }();
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".css"))  return "text/css";
    if (iequals(ext, ".js"))   return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".png"))  return "image/png";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg"))  return "image/jpeg";
    if (iequals(ext, ".gif"))  return "image/gif";
    if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if (iequals(ext, ".svg"))  return "image/svg+xml";
    return "application/text";
}

std::string pathJoin(boost::beast::string_view base, boost::beast::string_view path) {
    if (base.empty()) return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr pathSeparator = '\\';
    if (result.back() == pathSeparator) result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for (auto& c : result) if (c == '/') c = pathSeparator;
#else
    char constexpr pathSeparator = '/';
    if (result.back() == pathSeparator) result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

namespace http = boost::beast::http;

template<class Body, class Allocator, class Send> void handleRequest(boost::beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    auto const badRequest = [&req](boost::beast::string_view why) {
        http::response<http::string_body> res{ http::status::bad_request, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    auto const notFound = [&req](boost::beast::string_view target) {
        http::response<http::string_body> res{ http::status::not_found, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    auto const serverError = [&req](boost::beast::string_view what) {
        http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    if (req.method() != http::verb::get && req.method() != http::verb::head) return send(badRequest("Unknown HTTP-method"));

    if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != boost::beast::string_view::npos)
        return send(badRequest("Illegal request-target"));

    std::string path = pathJoin(doc_root, req.target());
    if (req.target().back() == '/') path.append("index.html");

    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    if (ec == boost::system::errc::no_such_file_or_directory) return send(notFound(req.target()));

    if (ec) return send(serverError(ec.message()));

    auto const size = body.size();

    if (req.method() == http::verb::head) {
        http::response<http::empty_body> res{ http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, getMimeType(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, getMimeType(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

struct HttpSession::SendLambda {
    HttpSession& self;

    explicit SendLambda(HttpSession& self) : self(self) { }

    template<bool isRequest, class Body, class Fields> void operator()(http::message<isRequest, Body, Fields>&& msg) const {
        auto sp = boost::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

        auto self2 = self.shared_from_this();
        http::async_write(self.stream, *sp, [self2, sp](boost::beast::error_code ec, std::size_t bytes) {
            self2->onWrite(ec, bytes, sp->need_eof());
        });
    }
};

HttpSession::HttpSession(boost::asio::ip::tcp::socket&& socket, boost::shared_ptr<SharedState> const& state): state(state), stream(boost::beast::unlimited_rate_policy(), std::move(socket)) {
}

void HttpSession::doRead() {
    parser.emplace();
    parser->body_limit(10000);
    stream.expires_after(std::chrono::seconds(30));
    http::async_read(stream, buffer, parser->get(), boost::beast::bind_front_handler(&HttpSession::onRead, shared_from_this()));
}

void HttpSession::onRead(boost::beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream) {
        stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        return;
    }

    if (ec) return;

    if (boost::beast::websocket::is_upgrade(parser->get())) {
        boost::make_shared<WebSocketSession>(stream.release_socket(), state)->run(parser->release());
        return;
    }

    handleRequest(state->getDocRoot(), parser->release(), SendLambda(*this));
}

void HttpSession::onWrite(boost::beast::error_code ec, std::size_t, bool close) {
    if (ec) return;
    if (close) stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    else doRead();
}