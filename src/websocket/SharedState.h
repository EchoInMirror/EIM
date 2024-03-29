#pragma once

#include <boost/smart_ptr.hpp>
#include <unordered_set>
#include <boost/beast.hpp>
#include <boost/beast/http/write.hpp>
#include <mutex>

class WebSocketSession;

class SharedState {
public:
    explicit SharedState(std::string docRoot) : docRoot(std::move(docRoot)) { }

    void join(WebSocketSession* session);
    void leave(WebSocketSession* session);
    void send(std::shared_ptr<boost::beast::flat_buffer> message);
    void sendExclude(std::shared_ptr<boost::beast::flat_buffer> message, WebSocketSession* session);
    int size();
    std::string const& getDocRoot() const noexcept { return docRoot; }
private:
    std::mutex mutex;
    std::string const docRoot;
    std::unordered_set<WebSocketSession*> sessions;
};
