#include "../Main.h"
#include "../websocket/WebSocketSession.h"

void loadPluginAndAdd(std::string identifier, bool setName, Track* track, std::function<void()>* callback) {
	if (identifier.empty()) return;
	auto instance = EIMApplication::getEIMInstance();
	auto type = instance->pluginManager->knownPluginList.getTypeForFile(identifier);
	if (setName) track->name = type->name.toStdString();
	if (type != nullptr) {
		instance->mainWindow->masterTrack->loadPlugin(std::move(type), [callback, track](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& err) {
			if (err.isEmpty()) {
				EIMApplication::getEIMInstance()->mainWindow->masterTrack->createPluginWindow(instance.get());
				if (instance->getPluginDescription().isInstrument) track->setInstrument(std::move(instance));
				else track->addEffectPlugin(std::move(instance));
			}
			if (callback != nullptr) {
				(*callback)();
				*callback = nullptr;
			}
		});
	}
};

class CreateTrackAction : public juce::UndoableAction {
private:
	std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data;
	std::function<void()> callback;
	bool hasPerformed;
public:
	CreateTrackAction(std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data) : data(std::move(data)), callback(nullptr) { }
	CreateTrackAction(std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data, std::function<void()> callback): data(std::move(data)), callback(callback) { }
	bool perform() override {
		auto instance = EIMApplication::getEIMInstance();
		auto track = (Track*)instance->mainWindow->masterTrack->createTrack(data->name(), data->color())->getProcessor();
		loadPluginAndAdd(data->identifier(), true, track, hasPerformed ? nullptr : &callback);
		if (!hasPerformed) hasPerformed = true;
		EIMPackets::ClientboundTracksInfo info;
		info.add_tracks()->CopyFrom(track->getTrackInfo());
		DBG("" << track->uuid);
		instance->listener->state->send(std::move(EIMMakePackets::makeSyncTracksInfoPacket(&info)));
		return true;
	}
	bool undo() override {
		// TODO
		return false;
	}
};

class UpdateTrackInfoAction : public juce::UndoableAction {
private:
	std::unique_ptr<EIMPackets::TrackInfo> data;
public:
	UpdateTrackInfoAction(std::unique_ptr<EIMPackets::TrackInfo> data) : data(std::move(data)) { }
	bool perform() override {
		auto instance = EIMApplication::getEIMInstance();
		auto& tracks = instance->mainWindow->masterTrack->tracksMap;
		auto& uuid = data->uuid();
		if (!tracks.contains(uuid)) return false;
		auto& trackPtr = tracks[uuid];
		auto track = (Track*)trackPtr->getProcessor();
		if (data->has_name()) track->name = data->name();
		if (data->has_color()) track->color = data->color();
		if (data->has_volume()) track->chain.get<1>().setGainLinear(data->volume());
		if (data->has_muted() && data->muted() != trackPtr->isBypassed()) track->setMuted(data->muted());
		EIMPackets::ClientboundTracksInfo info;
		info.add_tracks()->CopyFrom(track->getTrackInfo());
		instance->listener->state->send(std::move(EIMMakePackets::makeSyncTracksInfoPacket(&info)));
		return true;
	}
	bool undo() override {
		// TODO
		return false;
	}
};

void ServerService::handleCreateTrack(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data, std::function<void(EIMPackets::Empty&)> reply) {
	EIMApplication::getEIMInstance()->undoManager.perform(new CreateTrackAction(std::move(data), [reply] {
		EIMPackets::Empty out;
		reply(out);
	}));
}

void ServerService::handleUpdateTrackInfo(WebSocketSession*, std::unique_ptr<EIMPackets::TrackInfo> data) {
	EIMApplication::getEIMInstance()->undoManager.perform(new UpdateTrackInfoAction(std::move(data)));
}
