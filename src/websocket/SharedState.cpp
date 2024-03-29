#include "SharedState.h"
#include "../Main.h"
#include "../utils/Utils.h"
#include "WebSocketSession.h"
#include <boost/weak_ptr.hpp>

void SharedState::join(WebSocketSession* session) {
    std::lock_guard<std::mutex> lock(mutex);
    sessions.insert(session);
	runOnMainThread([this] {
		juce::String str("Current number of clients: ");
		str += (int)sessions.size();
		EIMApplication::getEIMInstance()->mainWindow->clientsLabel.setText(str, {});
	});
}

void SharedState::leave(WebSocketSession* session) {
    std::lock_guard<std::mutex> lock(mutex);
    sessions.erase(session);
	runOnMainThread([this] {
		juce::String str("Current number of clients: ");
		str += (int)sessions.size();
		EIMApplication::getEIMInstance()->mainWindow->clientsLabel.setText(str, {});
	});
}

void SharedState::send(std::shared_ptr<boost::beast::flat_buffer> message) {
    std::vector<boost::weak_ptr<WebSocketSession>> v;
    {
        std::lock_guard<std::mutex> lock(mutex);
        v.reserve(sessions.size());
        for (auto p : sessions) v.emplace_back(p->weak_from_this());
    }

    for (auto const& wp : v) if (auto sp = wp.lock()) sp->send(message);
}

void SharedState::sendExclude(std::shared_ptr<boost::beast::flat_buffer> message, WebSocketSession* session) {
    std::vector<boost::weak_ptr<WebSocketSession>> v;
    {
        std::lock_guard<std::mutex> lock(mutex);
        v.reserve(sessions.size());
        for (auto p : sessions) if (p != session) v.emplace_back(p->weak_from_this());
    }

    for (auto const& wp : v) if (auto sp = wp.lock()) sp->send(message);
}

int SharedState::size() { return (int)sessions.size(); }
