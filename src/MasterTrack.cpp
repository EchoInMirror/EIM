#include "MasterTrack.h"
#include "Main.h"
#include "../packets/packets.h"

MasterTrack::MasterTrack(): AudioProcessorGraph(), juce::AudioPlayHead() {
	setPlayHead(this);

	endTime = currentPositionInfo.timeSigNumerator * ppq;

	deviceManager.initialiseWithDefaultDevices(0, 2);
	setup = deviceManager.getAudioDeviceSetup();

	deviceManager.addAudioCallback(&graphPlayer);
	graphPlayer.setProcessor(this);
	auto output = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

	setPlayConfigDetails(0, 2, setup.sampleRate == 0 ? 48000 : setup.sampleRate, setup.bufferSize == 0 ? 1024 : setup.bufferSize);
	prepareToPlay(getSampleRate(), getBlockSize());

	outputNodeID = initTrack(std::make_unique<Track>("", this))->nodeID;
	addConnection({ { outputNodeID, 0 }, { output, 0 } });
	addConnection({ { outputNodeID, 1 }, { output, 1 } });
}

void MasterTrack::loadPlugin(std::unique_ptr<juce::PluginDescription> desc, juce::AudioPluginFormat::PluginCreationCallback callback) {
	EIMApplication::getEIMInstance()->pluginManager->manager.createPluginInstanceAsync(*desc, getSampleRate(), getBlockSize(), callback);
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::initTrack(std::unique_ptr<Track> track) {
	track->setRateAndBufferSizeDetails(getSampleRate(), getBlockSize());
	track->prepareToPlay(getSampleRate(), getBlockSize());
	auto& obj = *track;
	auto& node = tracks.emplace_back(addNode(std::move(track)));
	tracksMap[obj.uuid] = node;
	obj.currentNode = node;
	addConnection({ { node->nodeID, 0 }, { outputNodeID, 0 } });
	addConnection({ { node->nodeID, 1 }, { outputNodeID, 1 } });
	return node;
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::createTrack(std::string name, std::string color) {
	juce::MidiFile file;
	auto track = std::make_unique<Track>(name, color, this);
	auto env = juce::SystemStats::getEnvironmentVariable("MIDI_IMPORT_PATH", "");
	if (env.isNotEmpty()) {
		juce::FileInputStream theStream(env);
		file.readFrom(theStream);
		track->addMidiEvents(*file.getTrack(1), file.getTimeFormat());
		auto newEndTime = (int)std::ceil(juce::jmax(file.getTrack(1)->getEndTime() / file.getTimeFormat(), (double)currentPositionInfo.timeSigNumerator)) * ppq;
		if (endTime != newEndTime) {
			endTime = newEndTime;
			EIMPackets::ProjectStatus info;
			info.set_maxnotetime(endTime);
			EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
		}
	}
	return initTrack(std::move(track));
}

void MasterTrack::removeTrack(std::string uuid) {
	if (tracksMap.contains(uuid)) return;
	auto& it = tracksMap[uuid];
	removeNode(it);
	for (auto iter = tracks.begin(); iter != tracks.end(); iter++) if (*iter == it) {
		tracks.erase(iter);
		break;
	}
	tracksMap.erase(uuid);
}

bool MasterTrack::getCurrentPosition(CurrentPositionInfo& result) {
	result = currentPositionInfo;
	return true;
}

void MasterTrack::transportPlay(bool shouldStartPlaying) {
	if (currentPositionInfo.isPlaying == shouldStartPlaying) return;
	currentPositionInfo.isPlaying = shouldStartPlaying;
	EIMPackets::ProjectStatus info;
	info.set_isplaying(shouldStartPlaying);
	info.set_position((int)currentPositionInfo.ppqPosition);
	EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
}

void MasterTrack::calcPositionInfo() {
	if (!currentPositionInfo.isPlaying) return;
	currentPositionInfo.timeInSeconds = ((double)currentPositionInfo.timeInSamples) / getSampleRate();
	currentPositionInfo.ppqPosition = currentPositionInfo.timeInSeconds / 60.0 * currentPositionInfo.bpm * ppq;
	if (endTime >= currentPositionInfo.ppqPosition) return;
	currentPositionInfo.timeInSeconds = 0;
	currentPositionInfo.timeInSamples = 0;
	currentPositionInfo.ppqPosition = 0;
	EIMPackets::ProjectStatus info;
	info.set_position((int)currentPositionInfo.ppqPosition);
	EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
}

void MasterTrack::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	calcPositionInfo();
	AudioProcessorGraph::processBlock(buffer, midiMessages);
	if (currentPositionInfo.isPlaying) currentPositionInfo.timeInSamples += buffer.getNumSamples();
}

void MasterTrack::createPluginWindow(juce::AudioPluginInstance* instance) {
	if (!instance) return;
	if (pluginWindows.contains(instance)) {
		auto& it = pluginWindows.at(instance);
		it.setAlwaysOnTop(true);
		it.grabKeyboardFocus();
		it.setAlwaysOnTop(false);
	} else pluginWindows.try_emplace(instance, instance, &pluginWindows);
}

void MasterTrack::writeProjectStatus(EIMPackets::ProjectStatus& it) {
	it.set_bpm((int)currentPositionInfo.bpm);
	it.set_position((int)currentPositionInfo.ppqPosition);
	it.set_timesignumerator(currentPositionInfo.timeSigNumerator);
	it.set_timesigdenominator(currentPositionInfo.timeSigDenominator);
	it.set_ppq(ppq);
	it.set_isplaying(currentPositionInfo.isPlaying);
	it.set_maxnotetime(endTime);
}

void MasterTrack::stopAllNotes() {
	auto msg = juce::MidiMessage::allNotesOff(1);
	msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
	for (auto& it : tracks) ((Track*)it->getProcessor())->messageCollector.addMessageToQueue(msg);
}
