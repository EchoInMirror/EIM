#include "../Main.h"
#include "../utils/Utils.h"
#include "../websocket/WebSocketSession.h"

void loadPluginAndAdd(std::string identifier, bool setName, Track* track,
                      std::function<void(bool, juce::AudioPluginInstance*)> callback) {
    if (identifier.empty()) return;
    auto instance = EIMApplication::getEIMInstance();
    auto type = instance->pluginManager->knownPluginList.getTypeForFile(identifier);
    if (setName) track->name = type->name.toStdString();
    if (type == nullptr) callback(false, nullptr);
    else
        instance->mainWindow->masterTrack->loadPlugin(
            std::move(type),
            [callback, track](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& err) {
                if (err.isEmpty()) {
                    EIMApplication::getEIMInstance()->pluginManager->createPluginWindow(instance.get());
                    auto inst = instance.get();
                    if (instance->getPluginDescription().isInstrument) track->setInstrument(std::move(instance));
                    else
                        track->addEffectPlugin(std::move(instance));
                    callback(inst->getPluginDescription().isInstrument, inst);
                }
                else {
                    callback(false, nullptr);
                }
            });
};

class CreateTrackAction : public juce::UndoableAction {
private:
    std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data;
    std::function<void()> callback;
    std::string uuid = "";

public:
    CreateTrackAction(std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data)
        : data(std::move(data)), callback(nullptr) {
    }
    CreateTrackAction(std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data, std::function<void()> callback)
        : data(std::move(data)), callback(callback) {
    }
    bool perform() override {
        auto track = (Track*)EIMApplication::getEIMInstance()
                         ->mainWindow->masterTrack->createTrack(data->name(), data->color(), uuid)
                         ->getProcessor();
        uuid = track->uuid;
        if (data->has_identifier()) {
            loadPluginAndAdd(data->identifier(), true, track, [this, track](bool, juce::AudioPluginInstance*) {
                EIMPackets::ClientboundTracksInfo info;
                track->writeTrackInfo(info.add_tracks());
                EIMApplication::getEIMInstance()->listener->boardcast(
                    std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
                if (callback != nullptr) {
                    callback();
                    callback = nullptr;
                }
            });
        }
        else {
            EIMPackets::ClientboundTracksInfo info;
            track->writeTrackInfo(info.add_tracks());
            EIMApplication::getEIMInstance()->listener->boardcast(
                std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
        }
        return true;
    }
    bool undo() override {
		auto instance = EIMApplication::getEIMInstance();
		instance->mainWindow->masterTrack->removeTrack(uuid);
		EIMPackets::String msg;
		msg.set_value(uuid);
		instance->listener->boardcast(std::move(EIMMakePackets::makeRemoveTrackPacket(msg)));
        return true;
    }
};

void ServerService::handleCreateTrack(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data,
                                      std::function<void(EIMPackets::Empty&)> reply) {
    EIMApplication::getEIMInstance()->undoManager.perform(new CreateTrackAction(std::move(data), [reply] {
        EIMPackets::Empty out;
        reply(out);
    }), "CreateTrackAction");
	EIMApplication::getEIMInstance()->undoManager.beginNewTransaction();
}

class UpdateTrackInfoAction : public juce::UndoableAction {
private:
	EIMPackets::ClientboundTracksInfo preInfo;
	std::unique_ptr<EIMPackets::TrackInfo> data;

public:

    UpdateTrackInfoAction(std::unique_ptr<EIMPackets::TrackInfo> data) : data(std::move(data)) {
    }
    bool perform() override {
        auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
        auto& trackPtr = tracks[uuid];
        auto track = (Track*)trackPtr->getProcessor();
		auto preTrackInfo = preInfo.add_tracks();
		preTrackInfo->set_uuid(uuid);
		if (data->has_name()) {
			preTrackInfo->set_name(track->name);
			track->name = data->name();
		}
		if (data->has_color()) {
			preTrackInfo->set_color(track->color);
			track->color = data->color();
		}
		if (data->has_volume()) {
			preTrackInfo->set_volume(track->chain.get<1>().getGainLinear());
			track->chain.get<1>().setGainLinear(data->volume());
		}
		if (data->has_muted() && data->muted() != trackPtr->isBypassed()) {
			preTrackInfo->set_muted(trackPtr->isBypassed());
			track->setMuted(data->muted());
		}
		if (data->has_pan() && data->pan() != track->pan) {
			preTrackInfo->set_pan(track->pan);
			track->chain.get<0>().setPan(juce::jlimit(-100, 100, track->pan = data->pan()) / 100.0f);
		}
        if (!data->has_pan() && !data->has_volume()) {
            EIMPackets::ClientboundTracksInfo info;
            info.add_tracks()->CopyFrom(*data);
            instance->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
        }
        return true;
    }
    bool undo() override {
		auto instance = EIMApplication::getEIMInstance();
		auto& tracks = instance->mainWindow->masterTrack->tracksMap;
		auto& uuid = data->uuid();
		if (!tracks.contains(uuid)) return false;
		auto& trackPtr = tracks[uuid];
		auto track = (Track*)trackPtr->getProcessor();
		auto& info = preInfo.tracks(0);
		if (info.has_name()) track->name = info.name();
		if (info.has_color()) track->color = info.color();
		if (info.has_volume()) track->chain.get<1>().setGainLinear(info.volume());
		if (info.has_name()) track->name = info.name();
		if (info.has_muted()) track->setMuted(info.muted());
		if (info.has_pan()) track->chain.get<0>().setPan((track->pan = info.pan()) / 100.0f);
		instance->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(preInfo)));
        return true;
    }
	UndoableAction* createCoalescedAction(UndoableAction* nextAction) override {
		UpdateTrackInfoAction* next = dynamic_cast<UpdateTrackInfoAction*>(nextAction);
		if (next == nullptr || next->data->uuid() != data->uuid() ||
			data->has_name() || data->has_color() || data->has_solo() || data->has_muted() ||
			next->data->has_name() || next->data->has_color() || next->data->has_solo() || next->data->has_muted()) return nullptr;
		data->set_volume(data->volume() + next->data->volume());
		data->set_pan(data->pan() + next->data->pan());
		next = new UpdateTrackInfoAction(std::move(data));
		next->preInfo = preInfo;
		return next;
	}
};

void ServerService::handleUpdateTrackInfo(WebSocketSession*, std::unique_ptr<EIMPackets::TrackInfo> data) {
	auto flag = !data->has_volume() && !data->has_pan();
    EIMApplication::getEIMInstance()->undoManager.perform(new UpdateTrackInfoAction(std::move(data)), "UpdateTrackInfoAction");
	if (flag) EIMApplication::getEIMInstance()->undoManager.beginNewTransaction();
}

class LoadVSTAction : public juce::UndoableAction {
  private:
    std::unique_ptr<EIMPackets::ServerboundLoadVST> data;
    std::function<void()> callback;
    juce::AudioPluginInstance* _pluginInstance = nullptr;
    bool _isInstrument = false;

  public:
    LoadVSTAction(std::unique_ptr<EIMPackets::ServerboundLoadVST> data) : data(std::move(data)), callback(nullptr) {
    }
    LoadVSTAction(std::unique_ptr<EIMPackets::ServerboundLoadVST> data, std::function<void()> callback)
        : data(std::move(data)), callback(callback) {
    }
    bool perform() override {
        auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
        auto& trackPtr = tracks[uuid];
        auto track = (Track*)trackPtr->getProcessor();
        loadPluginAndAdd(data->identifier(), false, track,
                         [this, track](bool isInstrument, juce::AudioPluginInstance* instance) {
                             _isInstrument = isInstrument;
                             _pluginInstance = instance;
                             if (callback != nullptr) {
                                 callback();
                                 callback = nullptr;
                             }
                             EIMPackets::ClientboundTracksInfo info;
                             track->writeTrackInfo(info.add_tracks());
                             EIMApplication::getEIMInstance()->listener->boardcast(
                                 std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
                         });
        return true;
    }
    bool undo() override {
        if (_pluginInstance == nullptr) return false;
        auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
        auto& trackPtr = tracks[uuid];
        auto track = (Track*)trackPtr->getProcessor();
        if (_isInstrument) {
            track->setInstrument(std::unique_ptr<juce::AudioPluginInstance>(_pluginInstance));
        }
        else {
            track->removeEffectPlugin(_pluginInstance);
        }
		EIMPackets::ClientboundTracksInfo info;
		track->writeTrackInfo(info.add_tracks());
		instance->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
        return true;
    }
};

void ServerService::handleLoadVST(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundLoadVST> data,
                                  std::function<void(EIMPackets::Empty&)> reply) {
    EIMApplication::getEIMInstance()->undoManager.perform(new LoadVSTAction(std::move(data), [reply] {
        EIMPackets::Empty out;
        reply(out);
    }), "LoadVSTAction");
	EIMApplication::getEIMInstance()->undoManager.beginNewTransaction();
}

class AddMidiMessagesAction : public juce::UndoableAction {
  private:
    std::unique_ptr<EIMPackets::MidiMessages> data;
    juce::MidiMessageSequence seq;

  public:
    AddMidiMessagesAction(std::unique_ptr<EIMPackets::MidiMessages> data) : data(std::move(data)) {
    }
    bool perform() override {
        auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
        auto track = (Track*)tracks[uuid]->getProcessor();
        for (auto& it : data->midi())
            seq.addEvent(decodeMidiMessage(it.data(), it.time()));
        track->midiSequence.addSequence(seq, 0);
		instance->listener->boardcast(std::move(EIMMakePackets::makeAddMidiMessagesPacket(*data)));
        return true;
    }
    bool undo() override {
		auto instance = EIMApplication::getEIMInstance();
		auto& tracks = instance->mainWindow->masterTrack->tracksMap;
		auto& uuid = data->uuid();
		if (!tracks.contains(uuid)) return false;
		auto track = (Track*)tracks[uuid]->getProcessor();
		EIMPackets::MidiMessages msg;
		msg.set_uuid(uuid);
		for (auto& it : seq) {
			int id = track->midiSequence.getIndexOf(it);
			if (id == -1) continue;
			msg.add_data(id);
			track->midiSequence.deleteEvent(id, false);
		}
		instance->listener->boardcast(std::move(EIMMakePackets::makeDeleteMidiMessagesPacket(msg)));
        return true;
    }
};

void ServerService::handleAddMidiMessages(WebSocketSession*, std::unique_ptr<EIMPackets::MidiMessages> data) {
    EIMApplication::getEIMInstance()->undoManager.perform(new AddMidiMessagesAction(std::move(data)), "AddMidiMessagesAction");
	EIMApplication::getEIMInstance()->undoManager.beginNewTransaction();
}

class DeleteMidiMessagesAction : public juce::UndoableAction {
  private:
    std::unique_ptr<EIMPackets::MidiMessages> data;
    juce::MidiMessageSequence preSeq;

  public:
    DeleteMidiMessagesAction(std::unique_ptr<EIMPackets::MidiMessages> data) : data(std::move(data)) { }
    bool perform() override {
        auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
		preSeq.clear();
        auto track = (Track*)tracks[uuid]->getProcessor();
        int times = 0;
        int idx = 0;
        for (auto it : data->data()) {
            idx = it - times++;
            preSeq.addEvent(juce::MidiMessage(track->midiSequence.getEventPointer(idx)->message));
            track->midiSequence.deleteEvent(idx, false);
        }
		instance->listener->boardcast(std::move(EIMMakePackets::makeDeleteMidiMessagesPacket(*data)));
        return true;
    }
    bool undo() override {
		auto instance = EIMApplication::getEIMInstance();
		auto& tracks = instance->mainWindow->masterTrack->tracksMap;
		auto& uuid = data->uuid();
		if (!tracks.contains(uuid)) return false;
		EIMPackets::MidiMessages msg;
		msg.set_uuid(uuid);
		for (auto it : preSeq) {
			auto midi = msg.add_midi();
			midi->set_data(encodeMidiMessage(it->message));
			midi->set_time((int)it->message.getTimeStamp());
		}
		((Track*)tracks[uuid]->getProcessor())->midiSequence.addSequence(preSeq, 0);
		preSeq.clear();
		instance->listener->boardcast(std::move(EIMMakePackets::makeAddMidiMessagesPacket(msg)));
        return true;
    }
};

void ServerService::handleDeleteMidiMessages(WebSocketSession*, std::unique_ptr<EIMPackets::MidiMessages> data) {
    EIMApplication::getEIMInstance()->undoManager.perform(new DeleteMidiMessagesAction(std::move(data)), "DeleteMidiMessagesAction");
	EIMApplication::getEIMInstance()->undoManager.beginNewTransaction();
}

class EditMidiMessagesAction : public juce::UndoableAction {
  private:
    std::unique_ptr<EIMPackets::MidiMessages> data;
    std::vector<juce::MidiMessageSequence::MidiEventHolder*> selectedMessages;
	std::vector<juce::MidiMessage> messages;

  public:
    EditMidiMessagesAction(std::unique_ptr<EIMPackets::MidiMessages> data) : data(std::move(data)) {
    }
    bool perform() override {
        auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
        auto track = (Track*)tracks[uuid]->getProcessor();
        int index = 0;
        auto& ids = data->data();
		messages.clear();
		selectedMessages.clear();
        for (auto& it : data->midi()) {
            auto target = track->midiSequence.getEventPointer(ids[index++]);
			selectedMessages.push_back(target);
			messages.push_back(juce::MidiMessage(target->message));
			target->message = decodeMidiMessage(it.data(), it.time());
        }
        track->midiSequence.sort();
		instance->listener->boardcast(std::move(EIMMakePackets::makeEditMidiMessagesPacket(*data)));
        return true;
    }
    bool undo() override {
		auto instance = EIMApplication::getEIMInstance();
        auto& tracks = instance->mainWindow->masterTrack->tracksMap;
        auto& uuid = data->uuid();
        if (!tracks.contains(uuid)) return false;
        auto track = (Track*)tracks[uuid]->getProcessor();
		EIMPackets::MidiMessages msg;
		msg.set_uuid(uuid);
		int i = 0;
		for (auto it : selectedMessages) {
			auto id = track->midiSequence.getIndexOf(it);
			if (id == -1) continue;
			msg.add_data(id);
			it->message = messages[i++];
			auto midi = msg.add_midi();
			midi->set_data(encodeMidiMessage(it->message));
			midi->set_time((int)it->message.getTimeStamp());
		}
		selectedMessages.clear();
		messages.clear();
		track->midiSequence.sort();
		instance->listener->boardcast(std::move(EIMMakePackets::makeEditMidiMessagesPacket(msg)));
        return true;
    }
};

void ServerService::handleEditMidiMessages(WebSocketSession*, std::unique_ptr<EIMPackets::MidiMessages> data) {
    EIMApplication::getEIMInstance()->undoManager.perform(new EditMidiMessagesAction(std::move(data)), "EditMidiMessagesAction");
	EIMApplication::getEIMInstance()->undoManager.beginNewTransaction();
}
