#include "Track.h"
#include "ProcessorBase.h"

Track::Track(std::string name, std::string color): SynchronizedAudioProcessorGraph(), name(name), color(color) {
	setChannelLayoutOfBus(true, 0, juce::AudioChannelSet::canonicalChannelSet(2));
	setChannelLayoutOfBus(false, 0, juce::AudioChannelSet::canonicalChannelSet(2));
	auto input = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
	begin = addNode(std::make_unique<ProcessorBase>());
	end = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
	midiIn = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode))->nodeID;
	auto midiOut = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(juce::AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode))->nodeID;

	addConnection({ { input->nodeID, 0 }, { begin->nodeID, 0 } });
	addConnection({ { input->nodeID, 1 }, { begin->nodeID, 1 } });
	addConnection({ { begin->nodeID, 0 }, { end->nodeID, 0 } });
	addConnection({ { begin->nodeID, 1 }, { end->nodeID, 1 } });
	addConnection({ { midiIn, juce::AudioProcessorGraph::midiChannelIndex }, { midiOut, juce::AudioProcessorGraph::midiChannelIndex } });
}

void Track::setGenerator(std::unique_ptr<PluginWrapper> instance) {
	auto node = addNode(std::move(instance));
	addConnection({ { node->nodeID, 0 }, { end->nodeID, 0 } });
	addConnection({ { node->nodeID, 1 }, { end->nodeID, 1 } });
	addConnection({ { midiIn, juce::AudioProcessorGraph::midiChannelIndex }, { node->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
}

void Track::setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) {
	SynchronizedAudioProcessorGraph::setRateAndBufferSizeDetails(newSampleRate, newBlockSize);
	messageCollector.reset(newSampleRate);
}

void Track::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	juce::AudioPlayHead::CurrentPositionInfo thePositionInfo;
	auto head = getPlayHead();
	if (head) {
		head->getCurrentPosition(thePositionInfo);
		if (thePositionInfo.isPlaying) {
			auto startTime = thePositionInfo.timeInSeconds;
			auto endTime = startTime + buffer.getNumSamples() / getSampleRate();
			// auto sampleLength = 1.0 / getSampleRate();
			for (auto it = midiSequence.begin() + midiSequence.getNextIndexAtTime(startTime); it < midiSequence.begin() && (*it)->message.getTimeStamp() < endTime; it++) {
				auto samplePosition = (int)std::round(((*it)->message.getTimeStamp() - startTime) * getSampleRate());
				midiMessages.addEvent((*it)->message, samplePosition);
			}
		}
	}
	messageCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());
	SynchronizedAudioProcessorGraph::processBlock(buffer, midiMessages);
}

void Track::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) {
	juce::AudioPlayHead::CurrentPositionInfo thePositionInfo;
	getPlayHead()->getCurrentPosition(thePositionInfo);
	if (thePositionInfo.isPlaying) {
		auto startTime = thePositionInfo.timeInSeconds;
		auto endTime = startTime + buffer.getNumSamples() / getSampleRate();
		// auto sampleLength = 1.0 / getSampleRate();
		for (auto it = midiSequence.begin() + midiSequence.getNextIndexAtTime(startTime); it < midiSequence.begin() && (*it)->message.getTimeStamp() < endTime; it++) {
			auto samplePosition = (int) std::round(((*it)->message.getTimeStamp() - startTime) * getSampleRate());
			midiMessages.addEvent((*it)->message, samplePosition);
		}
	}
	messageCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());
	SynchronizedAudioProcessorGraph::processBlock(buffer, midiMessages);
}
