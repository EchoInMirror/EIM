#include "PluginWrapper.h"

PluginWrapper::PluginWrapper(std::unique_ptr<juce::AudioPluginInstance> instance) : juce::DocumentWindow(
	instance->getName(), juce::Colours::lightgrey, juce::DocumentWindow::closeButton), SynchronizedAudioProcessorGraph() {
	setChannelLayoutOfBus(true, 0, juce::AudioChannelSet::canonicalChannelSet(2));
	setChannelLayoutOfBus(false, 0, juce::AudioChannelSet::canonicalChannelSet(2));
	instance->enableAllBuses();
	auto inputChannels = instance->getTotalNumInputChannels();
	auto outputChannels = instance->getTotalNumOutputChannels();
	if (inputChannels) instance->setChannelLayoutOfBus(true, 0, juce::AudioChannelSet::canonicalChannelSet(inputChannels));
	if (outputChannels) instance->setChannelLayoutOfBus(false, 0, juce::AudioChannelSet::canonicalChannelSet(outputChannels));
	setContentOwned(instance->createEditorIfNeeded(), true);
	auto accetpsMidi = instance->acceptsMidi(), producesMidi = instance->producesMidi();
	pluginNode = addNode(std::move(instance));
	auto inputNode = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode))->nodeID;
	auto outputNode = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))->nodeID;
	auto midiInNode = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode))->nodeID;
	auto midiOutNode = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode))->nodeID;
	addConnection({ { midiInNode, juce::AudioProcessorGraph::midiChannelIndex }, { midiOutNode, juce::AudioProcessorGraph::midiChannelIndex } });
	if (accetpsMidi && producesMidi) {
		addConnection({ { midiInNode, juce::AudioProcessorGraph::midiChannelIndex }, { pluginNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
		addConnection({ { pluginNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { midiOutNode, juce::AudioProcessorGraph::midiChannelIndex } });
	} else if (accetpsMidi) {
		addConnection({ { midiInNode, juce::AudioProcessorGraph::midiChannelIndex }, { pluginNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
	} else if (producesMidi) {
		addConnection({ { midiInNode, juce::AudioProcessorGraph::midiChannelIndex }, { midiOutNode, juce::AudioProcessorGraph::midiChannelIndex } });
		addConnection({ { pluginNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex }, { midiOutNode, juce::AudioProcessorGraph::midiChannelIndex } });
	}
	addConnection({ { pluginNode->nodeID, 0 }, { outputNode, 0 } });
	addConnection({ { pluginNode->nodeID, 1 }, { outputNode, 1 } });
	if (inputChannels) {
		addConnection({ { inputNode, 0 }, { pluginNode->nodeID, 0 } });
		addConnection({ { inputNode, 1 }, { pluginNode->nodeID, 1 } });
	}

	setVisible(true);
}

int PluginWrapper::getDesktopWindowStyleFlags() const {
	return DocumentWindow::getDesktopWindowStyleFlags();
}

void PluginWrapper::closeButtonPressed() {
	setVisible(false);
}
