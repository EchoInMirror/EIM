#include "MasterTrack.h"

MasterTrack::MasterTrack(): SynchronizedAudioProcessorGraph() {
	manager.addDefaultFormats();

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

	setPlayConfigDetails(0, 2, sampleRate, bufferSize);
	prepareToPlay(sampleRate, bufferSize);

	/*auto track = createTrack()->getProcessor();
	((Track*)track)->setGenerator(loadPlugin(0));*/
}

void MasterTrack::scanPlugins() {
	auto file = "C:\\Program Files\\Common Files\\VST3\\Arturia\\pigments.vst3";
	bool flag = false;
	for (auto it : manager.getFormats()) {
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
	auto node = addNode(std::move(track));
	tracks.push_back(node);
	addConnection({ { node->nodeID, 0 }, { outputNodeID, 0 } });
	addConnection({ { node->nodeID, 1 }, { outputNodeID, 1 } });
	return node;
}

void MasterTrack::removeTrack(int id) {
	removeNode(tracks[id]);
}
