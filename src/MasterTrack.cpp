#include "MasterTrack.h"
#include "Main.h"

MasterTrack::MasterTrack(): SynchronizedAudioProcessorGraph(), juce::AudioPlayHead() {
	manager.addDefaultFormats();
	setPlayHead(this);

	knownPluginListXMLFile = juce::File::getCurrentWorkingDirectory().getChildFile("knownPlugins.xml");
	if (knownPluginListXMLFile.exists()) {
		auto xml = juce::XmlDocument::parse(knownPluginListXMLFile.loadFileAsString());
		knownPluginList.recreateFromXml(*xml.get());
		xml.reset();
	} else scanPlugins();

	setup = deviceManager.getAudioDeviceSetup();
	deviceManager.initialiseWithDefaultDevices(0, 2);

	deviceManager.addAudioCallback(&graphPlayer);
	graphPlayer.setProcessor(this);
	outputNodeID = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

	setPlayConfigDetails(0, 2, 96000, 1024);
	prepareToPlay(getSampleRate(), getBlockSize());
}

void MasterTrack::scanPlugins() {
	auto file = "C:/Program Files/Steinberg/VSTPlugins/Spire-1.5_x64/Spire-1.5.dll";
	bool flag = false;
	for (auto it : manager.getFormats()) {
		DBG("" << it->getName());
		if (it->fileMightContainThisPluginType(file)) {
			auto arr = new juce::OwnedArray<juce::PluginDescription>();
			if (knownPluginList.scanAndAddFile(file, true, *arr, *it)) {
				flag = true;
				break;
			}
		}
	}

	if (flag) {
		knownPluginList.createXml().release()->writeTo(knownPluginListXMLFile);
	}
}

std::unique_ptr<PluginWrapper> MasterTrack::loadPlugin(std::unique_ptr<juce::PluginDescription> desc) {
	juce::String err;
	return std::make_unique<PluginWrapper>(std::move(manager.createPluginInstance(*desc, getSampleRate(), getBlockSize(), err)));
}

void MasterTrack::loadPluginAsync(std::unique_ptr<juce::PluginDescription> desc, MasterTrack::PluginCreationCallback callback) {
	manager.createPluginInstanceAsync(*desc, getSampleRate(), getBlockSize(), [callback](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& err) {
		callback(std::make_unique<PluginWrapper>(std::move(instance)), err.toStdString());
	});
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::createTrack(std::string name, std::string color) {
	auto track = std::make_unique<Track>(name, color);
	track->setRateAndBufferSizeDetails(getSampleRate(), getBlockSize());
	track->prepareToPlay(getSampleRate(), getBlockSize());
	juce::FileInputStream theStream(juce::File("E:/Midis/UTMR&C VOL 1-14 [MIDI FILES] for other DAWs FINAL by Hunter UT/VOL 1/1. Sean Tyas & Darren Porter - Relentless LD.mid"));
	juce::MidiFile file;
	file.readFrom(theStream);
	file.convertTimestampTicksToSeconds();
	double sampleRate = getSampleRate();
	auto t = file.getTrack(1);
	for (int i = 0; i < t->getNumEvents(); i++) {
		auto& m = t->getEventPointer(i)->message;
		int sampleOffset = (int)(sampleRate * m.getTimeStamp());
		track->midiBuffer.addEvent(m, sampleOffset);
	}
	endTime = t->getEndTime();
	DBG("" << endTime);
	auto node = addNode(std::move(track));
	tracks.push_back(node);
	addConnection({ { node->nodeID, 0 }, { outputNodeID, 0 } });
	addConnection({ { node->nodeID, 1 }, { outputNodeID, 1 } });
	return node;
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
	startTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
	EIMApplication::getEIMInstance()->listener->broadcastProjectStatus();
}

void MasterTrack::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	if (currentPositionInfo.isPlaying) {
		auto flag = juce::jmax(endTime, currentPositionInfo.timeSigDenominator * currentPositionInfo.timeSigNumerator * 0.6) < currentPositionInfo.timeInSeconds;
		if (flag) startTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
		currentPositionInfo.ppqPosition = ((currentPositionInfo.timeInSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime) * currentPositionInfo.bpm) * 0.6;
		if (flag) {
			currentPositionInfo.timeInSamples = (juce::int64)(getSampleRate() * currentPositionInfo.timeInSeconds); // TODO
			EIMApplication::getEIMInstance()->listener->broadcastProjectStatus();
		}
	}
	SynchronizedAudioProcessorGraph::processBlock(buffer, midiMessages);
	if (currentPositionInfo.isPlaying) currentPositionInfo.timeInSamples += buffer.getNumSamples();
}

void MasterTrack::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) {
	if (currentPositionInfo.isPlaying) {
		currentPositionInfo.ppqPosition = ((currentPositionInfo.timeInSeconds = startTime - juce::Time::getMillisecondCounterHiRes() * 0.001) * currentPositionInfo.bpm) * 0.6;
		currentPositionInfo.timeInSamples = getSampleRate() * currentPositionInfo.timeInSeconds;
	}
	SynchronizedAudioProcessorGraph::processBlock(buffer, midiMessages);
}

void MasterTrack::stopAllNotes() {
	auto msg = juce::MidiMessage::allNotesOff(1);
	msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
	for (auto& it : tracks) ((Track*)it->getProcessor())->getMidiMessageCollector().addMessageToQueue(msg);
}
