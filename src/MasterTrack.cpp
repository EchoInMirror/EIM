#include "MasterTrack.h"

MasterTrack::MasterTrack(): SynchronizedAudioProcessorGraph() {
	manager.addDefaultFormats();

	knownPluginListXMLFile = juce::File::getCurrentWorkingDirectory().getChildFile("knownPlugins.xml");
	if (knownPluginListXMLFile.exists()) {
		auto xml = juce::XmlDocument::parse(knownPluginListXMLFile.loadFileAsString());
		list.recreateFromXml(*xml.get());
		xml.reset();
	} else scanPlugins();

	setup = deviceManager.getAudioDeviceSetup();
	deviceManager.initialiseWithDefaultDevices(0, 2);

	deviceManager.addAudioCallback(&graphPlayer);
	graphPlayer.setProcessor(this);
	outputNodeID = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;

	setPlayConfigDetails(0, 2, sampleRate, bufferSize);
	prepareToPlay(sampleRate, bufferSize);

	auto track = createTrack()->getProcessor();
	((Track*)track)->setGenerator(loadPlugin(0));
	startTimerHz(10);
}

void MasterTrack::timerCallback() {
	return;
	auto msg = juce::MidiMessage::noteOn(1, 60, (juce::uint8)120);
	msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
	((Track*)tracks[0]->getProcessor())->getMidiMessageCollector().addMessageToQueue(msg);
	auto msg2 = juce::MidiMessage::noteOff(1, 60);
	msg2.setTimeStamp((juce::Time::getMillisecondCounterHiRes() + 10) * 0.001);
	((Track*)tracks[0]->getProcessor())->getMidiMessageCollector().addMessageToQueue(msg2);
}

void MasterTrack::scanPlugins() {
	auto file = "C:\\Program Files\\Common Files\\VST3\\Arturia\\pigments.vst3";
	auto formats = manager.getFormats();
	bool flag = false;
	for (auto it = formats.begin(); it != formats.end(); it++) {
		if ((*it)->fileMightContainThisPluginType(file)) {
			auto arr = new juce::OwnedArray<juce::PluginDescription>();
			if (list.scanAndAddFile(file, true, *arr, **it)) {
				flag = true;
				break;
			}
		}
	}

	if (flag) {
		list.createXml().release()->writeTo(knownPluginListXMLFile);
	}
}

std::unique_ptr<PluginWrapper> MasterTrack::loadPlugin(int id) {
	juce::String err;
	auto instance = manager.createPluginInstance(*list.getType(id), getSampleRate(), getBlockSize(), err);
	return std::make_unique<PluginWrapper>(std::move(instance));
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::createTrack() {
	auto track = std::make_unique<Track>();
	track->setRateAndBufferSizeDetails(getSampleRate(), getBlockSize());
	track->prepareToPlay(getSampleRate(), getBlockSize());
	auto node = addNode(std::move(track));
	tracks.push_back(node);
	addConnection({ { node->nodeID, 0 }, { outputNodeID, 0 } });
	addConnection({ { node->nodeID, 1 }, { outputNodeID, 1 } });
	return node;
}

void MasterTrack::removeTrack(int id) {
	removeNode(tracks[id]);
}
