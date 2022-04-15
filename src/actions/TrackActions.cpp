#include "../Main.h"
#include "../websocket/WebSocketSession.h"
#include "../utils/Utils.h"

void loadPluginAndAdd(std::string identifier, bool setName, Track *track, std::function<void()> callback)
{
	if (identifier.empty())
		return;
	auto instance = EIMApplication::getEIMInstance();
	auto type = instance->pluginManager->knownPluginList.getTypeForFile(identifier);
	if (setName)
		track->name = type->name.toStdString();
	if (type == nullptr)
		callback();
	else
		instance->mainWindow->masterTrack->loadPlugin(std::move(type), [callback, track](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String &err)
													  {
		if (err.isEmpty()) {
			EIMApplication::getEIMInstance()->mainWindow->masterTrack->createPluginWindow(instance.get());
			if (instance->getPluginDescription().isInstrument) track->setInstrument(std::move(instance));
			else track->addEffectPlugin(std::move(instance));
		}
		callback(); });
};

class CreateTrackAction : public juce::UndoableAction
{
private:
	std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data;
	std::function<void()> callback;

public:
	CreateTrackAction(std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data) : data(std::move(data)), callback(nullptr) {}
	CreateTrackAction(std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data, std::function<void()> callback) : data(std::move(data)), callback(callback) {}
	bool perform() override
	{
		auto track = (Track *)EIMApplication::getEIMInstance()->mainWindow->masterTrack->createTrack(data->name(), data->color())->getProcessor();
		if (data->has_identifier())
		{
			loadPluginAndAdd(data->identifier(), true, track, [this, track]()
							 {
				EIMPackets::ClientboundTracksInfo info;
				track->writeTrackInfo(info.add_tracks());
				EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
				if (callback != nullptr) {
					callback();
					callback = nullptr;
				} });
		}
		else
		{
			EIMPackets::ClientboundTracksInfo info;
			track->writeTrackInfo(info.add_tracks());
			EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
		}
		return true;
	}
	bool undo() override
	{
		// TODO
		return false;
	}
};

void ServerService::handleCreateTrack(WebSocketSession *, std::unique_ptr<EIMPackets::ServerboundCreateTrackData> data, std::function<void(EIMPackets::Empty &)> reply)
{
	EIMApplication::getEIMInstance()->undoManager.perform(new CreateTrackAction(std::move(data), [reply]
																				{
		EIMPackets::Empty out;
		reply(out); }));
}

class UpdateTrackInfoAction : public juce::UndoableAction
{
private:
	std::unique_ptr<EIMPackets::TrackInfo> data;
	std::string _name;
	std::string _color;
	float _volume;
	bool _muted;
	int _pan;
	Track *preTrack = nullptr;
	EIMPackets::TrackInfo preTrackInfo;

public:
	UpdateTrackInfoAction(std::unique_ptr<EIMPackets::TrackInfo> data) : data(std::move(data)) {}
	bool perform() override
	{
		DBG("UpdateTrackInfoAction");
		auto instance = EIMApplication::getEIMInstance();
		auto &tracks = instance->mainWindow->masterTrack->tracksMap;
		auto &uuid = data->uuid();
		if (!tracks.contains(uuid))
			return false;
		auto &trackPtr = tracks[uuid];
		auto track = (Track *)trackPtr->getProcessor();
		preTrack = track;
		preTrackInfo.set_name(track->name);
		if (data->has_name())
			track->name = data->name();
		preTrackInfo.set_color(track->color);
		if (data->has_color())
			track->color = data->color();
		preTrackInfo.set_volume(track->chain.get<1>().getGainLinear());
		if (data->has_volume())
			track->chain.get<1>().setGainLinear(data->volume());
		preTrackInfo.set_muted(trackPtr->isBypassed());
		if (data->has_muted() && data->muted() != trackPtr->isBypassed())
			track->setMuted(data->muted());
		preTrackInfo.set_pan(track->pan);
		if (data->has_pan() && data->pan() != track->pan)
			track->chain.get<0>().setPan((track->pan = data->pan()) / 100.0f);
		if (!data->has_pan() && !data->has_volume())
		{
			EIMPackets::ClientboundTracksInfo info;
			info.add_tracks()->CopyFrom(*data);
			instance->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
		}

		return true;
	}
	bool undo() override
	{
		if (!preTrack)
			return false;
		DBG("UpdateTrackInfoAction undo");
		preTrack->name = preTrackInfo.name();
		preTrack->color = preTrackInfo.color();
		preTrack->chain.get<1>().setGainLinear(preTrackInfo.volume());
		preTrack->setMuted(preTrackInfo.muted());
		preTrack->chain.get<0>().setPan((preTrack->pan = preTrackInfo.pan()) / 100.0f);
		EIMPackets::ClientboundTracksInfo info;
		info.add_tracks()->CopyFrom(preTrackInfo);
		EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
		return true;
	}
};

void ServerService::handleUpdateTrackInfo(WebSocketSession *, std::unique_ptr<EIMPackets::TrackInfo> data)
{
	EIMApplication::getEIMInstance()->undoManager.perform(new UpdateTrackInfoAction(std::move(data)));
}

class LoadVSTAction : public juce::UndoableAction
{
private:
	std::unique_ptr<EIMPackets::ServerboundLoadVST> data;
	std::function<void()> callback;

public:
	LoadVSTAction(std::unique_ptr<EIMPackets::ServerboundLoadVST> data) : data(std::move(data)), callback(nullptr) {}
	LoadVSTAction(std::unique_ptr<EIMPackets::ServerboundLoadVST> data, std::function<void()> callback) : data(std::move(data)), callback(callback) {}
	bool perform() override
	{
		auto instance = EIMApplication::getEIMInstance();
		auto &tracks = instance->mainWindow->masterTrack->tracksMap;
		auto &uuid = data->uuid();
		if (!tracks.contains(uuid))
			return false;
		auto &trackPtr = tracks[uuid];
		auto track = (Track *)trackPtr->getProcessor();
		loadPluginAndAdd(data->identifier(), false, track, [this, track]()
						 {
			if (callback != nullptr) {
				callback();
				callback = nullptr;
			}
			EIMPackets::ClientboundTracksInfo info;
			track->writeTrackInfo(info.add_tracks());
			EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info))); });
		return true;
	}
	bool undo() override
	{
		// TODO
		return false;
	}
};

void ServerService::handleLoadVST(WebSocketSession *, std::unique_ptr<EIMPackets::ServerboundLoadVST> data, std::function<void(EIMPackets::Empty &)> reply)
{
	EIMApplication::getEIMInstance()->undoManager.perform(new LoadVSTAction(std::move(data), [reply]
																			{
		EIMPackets::Empty out;
		reply(out); }));
}

class AddMidiMessagesAction : public juce::UndoableAction
{
private:
	std::unique_ptr<EIMPackets::MidiMessages> data;

public:
	AddMidiMessagesAction(std::unique_ptr<EIMPackets::MidiMessages> data) : data(std::move(data)) {}
	bool perform() override
	{
		auto instance = EIMApplication::getEIMInstance();
		auto &tracks = instance->mainWindow->masterTrack->tracksMap;
		auto &uuid = data->uuid();
		if (!tracks.contains(uuid))
			return false;
		auto track = (Track *)tracks[uuid]->getProcessor();
		juce::MidiMessageSequence seq;
		for (auto &it : data->midi())
			seq.addEvent(decodeMidiMessage(it.data(), it.time()));
		track->midiSequence.addSequence(seq, 0);
		EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeAddMidiMessagesPacket(*data)));
		return true;
	}
	bool undo() override
	{
		// TODO
		return false;
	}
};

void ServerService::handleAddMidiMessages(WebSocketSession *, std::unique_ptr<EIMPackets::MidiMessages> data)
{
	EIMApplication::getEIMInstance()->undoManager.perform(new AddMidiMessagesAction(std::move(data)));
}

class DeleteMidiMessagesAction : public juce::UndoableAction
{
private:
	std::unique_ptr<EIMPackets::MidiMessages> data;

public:
	DeleteMidiMessagesAction(std::unique_ptr<EIMPackets::MidiMessages> data) : data(std::move(data)) {}
	bool perform() override
	{
		auto instance = EIMApplication::getEIMInstance();
		auto &tracks = instance->mainWindow->masterTrack->tracksMap;
		auto &uuid = data->uuid();
		if (!tracks.contains(uuid))
			return false;
		auto track = (Track *)tracks[uuid]->getProcessor();
		int times = 0;
		for (auto it : data->data())
			track->midiSequence.deleteEvent(it - times++, false);
		EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeDeleteMidiMessagesPacket(*data)));
		return true;
	}
	bool undo() override
	{
		// TODO
		return false;
	}
};

void ServerService::handleDeleteMidiMessages(WebSocketSession *, std::unique_ptr<EIMPackets::MidiMessages> data)
{
	EIMApplication::getEIMInstance()->undoManager.perform(new DeleteMidiMessagesAction(std::move(data)));
}

class EditMidiMessagesAction : public juce::UndoableAction
{
private:
	std::unique_ptr<EIMPackets::MidiMessages> data;

public:
	EditMidiMessagesAction(std::unique_ptr<EIMPackets::MidiMessages> data) : data(std::move(data)) {}
	bool perform() override
	{
		auto instance = EIMApplication::getEIMInstance();
		auto &tracks = instance->mainWindow->masterTrack->tracksMap;
		auto &uuid = data->uuid();
		if (!tracks.contains(uuid))
			return false;
		auto track = (Track *)tracks[uuid]->getProcessor();
		int index = 0;
		auto &ids = data->data();
		for (auto &it : data->midi())
			track->midiSequence.getEventPointer(ids[index++])->message = decodeMidiMessage(it.data(), it.time());
		track->midiSequence.sort();
		EIMApplication::getEIMInstance()->listener->boardcast(std::move(EIMMakePackets::makeEditMidiMessagesPacket(*data)));
		return true;
	}
	bool undo() override
	{
		// TODO
		return false;
	}
};

void ServerService::handleEditMidiMessages(WebSocketSession *, std::unique_ptr<EIMPackets::MidiMessages> data)
{
	EIMApplication::getEIMInstance()->undoManager.perform(new EditMidiMessagesAction(std::move(data)));
}

void ServerService::handleUndo(WebSocketSession *)
{
	DBG("undo");
	EIMApplication::getEIMInstance()->undoManager.undo();
}