#include "MasterTrack.h"
#include "Main.h"

MasterTrack::MasterTrack(): AudioProcessorGraph(), juce::AudioPlayHead() {
	setPlayHead(this);

	endTime = currentPositionInfo.timeSigNumerator * ppq;

	setup = deviceManager.getAudioDeviceSetup();
	deviceManager.initialiseWithDefaultDevices(0, 2);

	deviceManager.addAudioCallback(&graphPlayer);
	graphPlayer.setProcessor(this);
	auto output = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

	setPlayConfigDetails(0, 2, setup.sampleRate == 0 ? 48000 : setup.sampleRate, setup.bufferSize == 0 ? 1024 : setup.bufferSize);
	prepareToPlay(getSampleRate(), getBlockSize());

	outputNodeID = initTrack(std::make_unique<Track>(juce::Uuid::null(), this))->nodeID;
	addConnection({ { outputNodeID, 0 }, { output, 0 } });
	addConnection({ { outputNodeID, 1 }, { output, 1 } });
}

void MasterTrack::loadPlugin(std::unique_ptr<juce::PluginDescription> desc, MasterTrack::PluginCreationCallback callback) {
	EIMApplication::getEIMInstance()->pluginManager->manager.createPluginInstanceAsync(*desc, getSampleRate(), getBlockSize(), [callback](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& err) {
		callback(std::make_unique<PluginWrapper>(std::move(instance)), err.toStdString());
	});
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::initTrack(std::unique_ptr<Track> track) {
	track->setRateAndBufferSizeDetails(getSampleRate(), getBlockSize());
	track->prepareToPlay(getSampleRate(), getBlockSize());
	auto& obj = *track;
	auto& node = tracks.emplace_back(addNode(std::move(track)));
	obj.currentNode = node;
	addConnection({ { node->nodeID, 0 }, { outputNodeID, 0 } });
	addConnection({ { node->nodeID, 1 }, { outputNodeID, 1 } });
	return node;
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::createTrack(std::string name, std::string color) {
	juce::MidiFile file;
	auto track = std::make_unique<Track>(name, color, this);
	juce::FileInputStream theStream(juce::File("E:/Midis/UTMR&C VOL 1-14 [MIDI FILES] for other DAWs FINAL by Hunter UT/VOL 1/1. Sean Tyas & Darren Porter - Relentless LD.mid"));
	file.readFrom(theStream);
	file.getTimeFormat();
	track->addMidiEvents(*file.getTrack(1), file.getTimeFormat());
	endTime = juce::jmax(file.getTrack(1)->getEndTime() / file.getTimeFormat(), (double)currentPositionInfo.timeSigNumerator) * ppq;
	return initTrack(std::move(track));
}

void MasterTrack::removeTrack(int id) {
	removeNode(tracks[id]);
}

bool MasterTrack::getCurrentPosition(CurrentPositionInfo& result) {
	result = currentPositionInfo;
	return true;
}

void MasterTrack::transportPlay(bool shouldStartPlaying) {
	if (currentPositionInfo.isPlaying == shouldStartPlaying) return;
	currentPositionInfo.isPlaying = shouldStartPlaying;
	EIMApplication::getEIMInstance()->listener->broadcastProjectStatus();
}

void MasterTrack::calcPositionInfo() {
	if (!currentPositionInfo.isPlaying) return;
	currentPositionInfo.timeInSeconds = ((double)currentPositionInfo.timeInSamples) / getSampleRate();
	currentPositionInfo.ppqPosition = currentPositionInfo.timeInSeconds / 60.0 * currentPositionInfo.bpm * ppq;
	if (endTime >= currentPositionInfo.ppqPosition) return;
	currentPositionInfo.timeInSeconds = 0;
	currentPositionInfo.timeInSamples = 0;
	currentPositionInfo.ppqPosition = 0;
	EIMApplication::getEIMInstance()->listener->broadcastProjectStatus();
}

void MasterTrack::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	calcPositionInfo();
	AudioProcessorGraph::processBlock(buffer, midiMessages);
	if (currentPositionInfo.isPlaying) currentPositionInfo.timeInSamples += buffer.getNumSamples();
}

void MasterTrack::stopAllNotes() {
	auto msg = juce::MidiMessage::allNotesOff(1);
	msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
	for (auto& it : tracks) ((Track*)it->getProcessor())->messageCollector.addMessageToQueue(msg);
}
