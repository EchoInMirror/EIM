#include "Main.h"
#include "websocket/WebSocketSession.h"

void ServerService::handleSetProjectStatus(WebSocketSession*, std::unique_ptr<EIMPackets::ProjectStatus> data) {
	auto shouldUpdate = false;
	auto instance = EIMApplication::getEIMInstance();
	auto& master = instance->mainWindow->masterTrack;
	auto& info = master->currentPositionInfo;
	if (data->has_bpm() && data->bpm() > 10) {
		info.bpm = data->bpm();
		shouldUpdate = true;
	}
	if (data->has_position()) {
		info.ppqPosition = data->position();
		info.timeInSeconds = info.ppqPosition * 60.0 / info.bpm / master->ppq;
		info.timeInSamples = (juce::int64)(master->getSampleRate() * info.timeInSeconds);
		shouldUpdate = true;
	}
	if (data->has_isplaying() && data->isplaying() != info.isPlaying) {
		info.isPlaying = data->isplaying();
		if (!data->isplaying()) master->stopAllNotes();
		shouldUpdate = true;
	}
	if (data->has_timesignumerator()) {
		info.timeSigNumerator = data->timesignumerator();
		shouldUpdate = true;
	}
	if (data->has_timesigdenominator()) {
		info.timeSigDenominator = data->timesigdenominator();
		shouldUpdate = true;
	}
	data->set_position((int)info.ppqPosition);
	if (shouldUpdate) instance->listener->boardcast(std::move(EIMMakePackets::makeSetProjectStatusPacket(*data)));
}

void ServerService::handleGetExplorerData(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundExplorerData> data, std::function<void(EIMPackets::ClientboundExplorerData&)> reply) {
	auto& path = data->path();
	auto instance = EIMApplication::getEIMInstance();
	EIMPackets::ClientboundExplorerData out;
	switch (data->type()) {
	case EIMPackets::ServerboundExplorerData::ExplorerType::ServerboundExplorerData_ExplorerType_PLUGINS:
		if (path.empty()) {
			std::unordered_set<juce::String> map;
			for (auto& it : instance->pluginManager->knownPluginList.getTypes()) map.emplace(it.manufacturerName);
			for (auto& it : map) out.add_folders(it.toStdString());
		}
		else {
			std::vector<juce::String> arr;
			for (auto& it : instance->pluginManager->knownPluginList.getTypes()) {
				if (path == it.manufacturerName) arr.emplace_back((it.isInstrument ? "I#" : "") + it.name +
					(it.pluginFormatName == "VST" ? " (VST)" : "") + "#EIM#" + it.fileOrIdentifier);
			}
			for (auto& it : arr) out.add_files(it.toStdString());
		}
	}
	reply(out);
}

void ServerService::handleRefresh(WebSocketSession* session) {
	session->send(EIMMakePackets::makeSetProjectStatusPacket(*EIMApplication::getEIMInstance()->mainWindow->masterTrack->getProjectStatus()));
	EIMPackets::ClientboundTracksInfo info;
	info.set_isreplacing(true);
	auto instance = EIMApplication::getEIMInstance();
	for (auto& track : instance->mainWindow->masterTrack->tracks) info.add_tracks()->CopyFrom(((Track*)track->getProcessor())->getTrackInfo());
	session->send(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
}

void ServerService::handleOpenPluginWindow(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundOpenPluginWindow> data) {
	auto& tracks = EIMApplication::getEIMInstance()->mainWindow->masterTrack->tracksMap;
	auto& uuid = data->uuid();
	if (!tracks.contains(uuid)) return;
	auto track = (Track*) tracks[uuid]->getProcessor();
	auto plugin = data->has_index()
		? track->plugins.size() > data->index() ? (juce::AudioPluginInstance*)track->plugins[data->index()]->getProcessor() : nullptr
		: track->getInstrumentInstance();
	if (plugin) juce::MessageManager::callAsync([plugin] { EIMApplication::getEIMInstance()->mainWindow->masterTrack->createPluginWindow(plugin); });
}

/*
void EIMApplication::handlePacket(WebSocketSession* session) {
	auto &buf = session->buffer;
	auto instance = EIMApplication::getEIMInstance();



	switch (buf.readUInt8()) {
	case ServerboundPacket::ServerboundReply: break;
	case ServerboundPacket::ServerboundSetProjectStatus: {
		auto bpm = buf.readDouble();
		auto time = buf.readDouble();
		auto isPlaying = buf.readBoolean();
		auto timeSigNumerator = buf.readUInt8();
		auto timeSigDenominator = buf.readUInt8();
		buf.readUInt16();
		auto shouldUpdate = false;
		if (bpm > 10) {
			info.bpm = bpm;
			shouldUpdate = true;
		}
		if (time > -1) {
			info.timeInSeconds = time;
			info.timeInSamples = (juce::int64)(master->getSampleRate() * time);
			shouldUpdate = true;
		}
		if (isPlaying != info.isPlaying) {
			info.isPlaying = isPlaying;
			info.timeInSamples = (juce::int64)(master->getSampleRate() * info.timeInSeconds);
			if (!isPlaying) master->stopAllNotes();
			shouldUpdate = true;
		}
		if (timeSigNumerator > 0) {
			info.timeSigNumerator = timeSigNumerator;
			shouldUpdate = true;
		}
		if (timeSigDenominator > 0) {
			info.timeSigDenominator = timeSigDenominator;
			shouldUpdate = true;
		}
		if (shouldUpdate) listener->broadcastProjectStatus();
		break;
	}
	case ServerboundPacket::ServerboundLoadPlugin: {
		auto replyId = buf.readInt32();
		auto id = buf.readUInt8();
		buf.readUInt8();
		auto& tracks = mainWindow->masterTrack->tracks;
		if (tracks.size() <= id) return;
		auto identifier = buf.readString();
		loadPluginAndAdd(identifier, replyId, false, (Track*)tracks[id]->getProcessor());
		listener->syncTrackInfo();
		break;
	}
	case ServerboundPacket::ServerboundRefresh:
		listener->syncTrackInfo();
		break;
	case ServerboundPacket::ServerboundMidiMessage: {
		auto id = buf.readUInt8();
		auto byte1 = buf.readUInt8();
		auto byte2 = buf.readUInt8();
		auto byte3 = buf.readUInt8();
		auto& tracks = mainWindow->masterTrack->tracks;
		if (tracks.size() <= id) return;
		((Track*)tracks[id]->getProcessor())->messageCollector.addMessageToQueue(juce::MidiMessage(byte1, byte2, byte3, juce::Time::getMillisecondCounterHiRes() * 0.001));
		break;
	}
	case ServerboundPacket::ServerboundUpdateTrackInfo: {
	}
	case ServerboundPacket::ServerboundMidiNotesAdd: {
		auto id = buf.readUInt8();
		auto& masterTrack = mainWindow->masterTrack;
		auto& tracks = masterTrack->tracks;
		if (tracks.size() <= id) return;
		auto& midiSequence = ((Track*)tracks[id]->getProcessor())->midiSequence;
		int len = buf.readUInt16();
		while (len-- > 0) {
			int note = buf.readUInt8();
			auto noteOn = juce::MidiMessage::noteOn(1, note, buf.readUInt8());
			int start = buf.readUInt32();
			noteOn.setTimeStamp(start);
			auto noteOff = juce::MidiMessage::noteOff(1, note);
			noteOff.setTimeStamp(start + buf.readUInt32());
			midiSequence.addEvent(noteOn)->noteOffObject = midiSequence.addEvent(noteOff);
		}
		masterTrack->endTime = juce::jmax(midiSequence.getEndTime(), (double)masterTrack->currentPositionInfo.timeSigNumerator * masterTrack->ppq);
		break;
	}
	case ServerboundPacket::ServerboundMidiNotesDelete: {
		auto id = buf.readUInt8();
		auto& masterTrack = mainWindow->masterTrack;
		auto& tracks = masterTrack->tracks;
		if (tracks.size() <= id) return;
		auto track = ((Track*)tracks[id]->getProcessor());
		auto& midiSequence = track->midiSequence;
		int len = buf.readUInt16(), note = buf.readUInt8(), time = buf.readUInt32();
		auto& info = masterTrack->currentPositionInfo;
		for (int i = 0; i < midiSequence.getNumEvents() && len > 0; i++) {
			auto it = midiSequence.getEventPointer(i);
			if (!it->message.isNoteOn() || (int)it->message.getTimeStamp() != time || it->message.getNoteNumber() != note) continue;
			if (info.isPlaying && it->noteOffObject && info.ppqPosition >= it->message.getTimeStamp() && info.ppqPosition <= it->noteOffObject->message.getTimeStamp()) {
				auto msg = juce::MidiMessage::noteOff(1, note);
				msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
				track->messageCollector.addMessageToQueue(msg);
			}
			midiSequence.deleteEvent(i--, true);
			if (--len > 0) {
				note = buf.readUInt8();
				time = buf.readUInt32();
			}
		}
		masterTrack->endTime = juce::jmax(midiSequence.getEndTime(), (double)info.timeSigNumerator * masterTrack->ppq);
		break;
	}
	case ServerboundPacket::ServerboundMidiNotesEdit: {
		auto id = buf.readUInt8();
		auto& masterTrack = mainWindow->masterTrack;
		auto& tracks = masterTrack->tracks;
		if (tracks.size() <= id) return;
		auto track = ((Track*)tracks[id]->getProcessor());
		auto& midiSequence = track->midiSequence;
		int len = buf.readUInt16();
		int dx = buf.readInt32(), dy = buf.readInt8(), dw = buf.readInt32();
		float dv = buf.readFloat();
		int note = buf.readUInt8(), time = buf.readUInt32();
		auto& info = masterTrack->currentPositionInfo;
		for (auto it = midiSequence.begin(); it != midiSequence.end() && len > 0; it++) {
			auto& msg = (*it)->message;
			auto curTime = (int)msg.getTimeStamp();
			if (!msg.isNoteOn() || curTime != time || msg.getNoteNumber() != note) continue;
			auto& noteOff = (*it)->noteOffObject;
			if (info.isPlaying && noteOff && info.ppqPosition >= curTime && info.ppqPosition <= noteOff->message.getTimeStamp()) {
				auto offMsg = juce::MidiMessage::noteOff(1, note);
				offMsg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
				track->messageCollector.addMessageToQueue(offMsg);
			}
			if (dx) msg.setTimeStamp(juce::jmax(0, curTime + dx));
			if (noteOff && (dx || dw)) noteOff->message.setTimeStamp(juce::jmax(0, (int)noteOff->message.getTimeStamp() + dx + dw));
			if (dy) {
				note = juce::jmin(juce::jmax(0, note + dy), 127);
				msg.setNoteNumber(note);
				if (noteOff) noteOff->message.setNoteNumber(note);
			}
			if (dv) msg.setVelocity(juce::jlimit<float>(0, 1, msg.getVelocity() + dv));
			if (--len > 0) {
				note = buf.readUInt8();
				time = buf.readUInt32();
			}
		}
		midiSequence.sort();
		masterTrack->endTime = juce::jmax(midiSequence.getEndTime(), (double)info.timeSigNumerator * masterTrack->ppq);
		break;
	}
	case ServerboundPacket::ServerboundOpenPluginManager:
		juce::MessageManager::callAsync([] { EIMApplication::getEIMInstance()->pluginManager->setVisible(true); });
		break;
	case ServerboundPacket::ServerboundConfig: {
		auto out = EIMPackets::makeReplyPacket(buf.readUInt32());
		auto& cfg = instance->config;
		auto& manager = instance->pluginManager;
		if (buf.readBoolean()) {
			cfg.config = juce::JSON::parse(buf.readString());
			out->writeString("");
		} else {
			auto obj = new juce::DynamicObject();
			juce::var map(obj);
			for (auto it : manager->manager.getFormats()) {
				auto paths = juce::PluginListComponent::getLastSearchPath(manager->scanningProperties, *it);
				juce::StringArray arr;
				for (int i = 0; i < paths.getNumPaths(); i++) arr.add(paths[i].getFullPathName());
				obj->setProperty(it->getName(), arr);
			}
			juce::var copy = cfg.config;
			copy.getDynamicObject()->setProperty("vstSearchPaths", map);
			out->writeString(juce::JSON::toString(copy));
		}
		session->send(out);
		break;
	}
	case ServerboundPacket::ServerboundScanVSTs:
		juce::MessageManager::callAsync([] { EIMApplication::getEIMInstance()->pluginManager->scanPlugins(); });
		break;
	case ServerboundPacket::ServerboundTrackMixerInfo:
		session->send(EIMPackets::makeAllTrackMixerInfoPacket());
		break;
	case ServerboundPacket::ServerboundOpenPluginWindow: {
		auto id = buf.readUInt8();
		auto& tracks = mainWindow->masterTrack->tracks;
		if (tracks.size() <= id) return;
		auto track = ((Track*)tracks[id]->getProcessor());
		id = buf.readUInt8();
		auto plugin = id == 255
			? track->getInstrumentInstance()
			: track->plugins.size() > id ? (juce::AudioPluginInstance*)track->plugins[id]->getProcessor() : nullptr;
		if (plugin) juce::MessageManager::callAsync([this, plugin] { mainWindow->masterTrack->createPluginWindow(plugin); });
		break;
	}
	}
}
*/
