#include "Main.h"
#include "websocket/Packets.h"
#include "websocket/WebSocketSession.h"

void GuiAppApplication::handlePacket(WebSocketSession* session) {
	auto &buf = session->buffer;
	switch (buf.readUInt8()) {
	case ServerboundPacket::ServerboundReply:
		break;
	case ServerboundPacket::ServerboundGetExplorerData: {
		auto replyId = buf.readUInt32();
		auto type = buf.readUInt8();
		auto path = buf.readString();
		auto out = makeReplyPacket(replyId);
		switch (type) {
		case 1:
			if (path.empty()) {
				std::unordered_set<juce::String> map;
				for (auto& it : mainWindow->masterTrack->knownPluginList.getTypes()) map.emplace(it.manufacturerName);
				out->writeUInt32(map.size());
				for (auto& it : map) out->writeString(it);
				out->writeUInt32(0);
			} else {
				out->writeUInt32(0);
				std::vector<juce::String> arr;
				for (auto& it : mainWindow->masterTrack->knownPluginList.getTypes()) {
					if (path == it.manufacturerName) arr.emplace_back(it.name + "#EIM#" + it.fileOrIdentifier);
				}
				out->writeUInt32(arr.size());
				for (auto& it : arr) out->writeString(it);
			}
			session->send(out);
		}
		break;
	}
	case ServerboundPacket::ServerboundCreateTrack: {
		auto replyId = buf.readInt32();
		auto name = buf.readString();
		auto color = buf.readString();
		auto pos = buf.readUInt8();
		auto identifier = buf.readString();
		auto track = mainWindow->masterTrack->createTrack(name, color);
		if (!identifier.empty()) {
			auto type = mainWindow->masterTrack->knownPluginList.getTypeForFile(identifier);
			if (type != nullptr) {
				auto track0 = (Track*)track->getProcessor();
				track0->name = type->name.toStdString();
				mainWindow->masterTrack->loadPluginAsync(std::move(type), [track0, session, replyId](std::unique_ptr<PluginWrapper> instance, const std::string& err) {
					auto out = makeReplyPacket(replyId);
					if (err.empty()) track0->setGenerator(std::move(instance));
					out->writeString(err);
					session->send(out);
				});
			}
		}
		syncTrackInfo();
		break;
	}
	case ServerboundPacket::ServerboundRefresh:
		syncTrackInfo();
		break;
	}
}

void GuiAppApplication::syncTrackInfo() {
	auto &tracks = mainWindow->masterTrack->tracks;
	auto buf = makePacket(ClientboundPacket::ClientboundSyncTrackInfo);
	buf->writeInt8(tracks.size());
	for (auto& it : tracks) {
		auto track = (Track*) it->getProcessor();
		buf->writeString(track->uuid.toString());
		buf->writeString(track->name);
		buf->writeString(track->color);
		buf->writeUInt8(80);
		buf->writeUInt8(false);
		buf->writeUInt8(false);
	}
	listener->state->send(buf);
}
