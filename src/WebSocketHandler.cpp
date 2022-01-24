#include "Main.h"
#include "websocket/Packets.h"
#include "websocket/WebSocketSession.h"

void EIMApplication::handlePacket(WebSocketSession* session) {
	auto &buf = session->buffer;
	switch (buf.readUInt8()) {
	case ServerboundPacket::ServerboundReply: break;
	case ServerboundPacket::ServerboundSetProjectStatus: {
		auto& master = mainWindow->masterTrack;
		auto& info = master->currentPositionInfo;
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
				out->writeUInt32((unsigned long)map.size());
				for (auto& it : map) out->writeString(it);
				out->writeUInt32(0);
			} else {
				out->writeUInt32(0);
				std::vector<juce::String> arr;
				for (auto& it : mainWindow->masterTrack->knownPluginList.getTypes()) {
					if (path == it.manufacturerName) arr.emplace_back(it.name + "#EIM#" + it.fileOrIdentifier);
				}
				out->writeUInt32((unsigned long)arr.size());
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
		buf.readUInt8();
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
		auto id = buf.readUInt8();
		auto& tracks = mainWindow->masterTrack->tracks;
		if (tracks.size() <= id) return;
		auto track = (Track*)tracks[id]->getProcessor();
		auto name = buf.readString();
		if (!name.empty()) track->name = name;
		auto color = buf.readString();
		auto flag = false;
		if (!color.empty()) {
			track->color = color;
			flag = true;
		}
		auto volume = buf.readFloat();
		if (volume > -1) track->chain.get<1>().setGainLinear(volume);
		auto muted = buf.readBoolean();
		if (muted != tracks[id]->isBypassed()) {
			track->setMuted(muted);
			flag = true;
		}
		buf.readBoolean();
		if (flag || session->state->size() > 1) {
			auto out = makePacket(ClientboundPacket::ClientboundUpdateTrackInfo);
			out->writeUInt8(id);
			track->writeTrackInfo(out.get());
			if (flag) session->state->send(out);
			else session->state->sendExclude(out, session);
		}
		break;
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
	}
}
