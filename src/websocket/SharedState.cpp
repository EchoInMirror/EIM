#include "SharedState.h"
#include "WebSocketSession.h"
#include <boost/weak_ptr.hpp>

void SharedState::join(WebSocketSession* session) {
    std::lock_guard<std::mutex> lock(mutex);
    sessions.insert(session);
}

void SharedState::leave(WebSocketSession* session) {
    std::lock_guard<std::mutex> lock(mutex);
    sessions.erase(session);
}

void SharedState::send(boost::shared_ptr<ByteBuffer> message) {
    std::vector<boost::weak_ptr<WebSocketSession>> v;
    {
        std::lock_guard<std::mutex> lock(mutex);
        v.reserve(sessions.size());
        for (auto p : sessions) v.emplace_back(p->weak_from_this());
    }

    for (auto const& wp : v) if (auto sp = wp.lock()) sp->send(message);
}

void SharedState::sendExclude(boost::shared_ptr<ByteBuffer> message, WebSocketSession* session) {
    std::vector<boost::weak_ptr<WebSocketSession>> v;
    {
        std::lock_guard<std::mutex> lock(mutex);
        v.reserve(sessions.size());
        for (auto p : sessions) if (p != session) v.emplace_back(p->weak_from_this());
    }

    for (auto const& wp : v) if (auto sp = wp.lock()) sp->send(message);
}

int SharedState::size() { return sessions.size(); }
