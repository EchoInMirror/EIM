#pragma once

#include "PluginWrapper.h"
#include "SynchronizedAudioProcessorGraph.h"
#include <juce_audio_utils/juce_audio_utils.h>

class Track: public SynchronizedAudioProcessorGraph {
public:
    Track(std::string name, std::string color);
    juce::Uuid uuid;
    std::string name;
    std::string color;

    juce::MidiBuffer midiBuffer;
    juce::MidiMessageSequence midiSequence;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;
    void setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) override;

    void setGenerator(std::unique_ptr<PluginWrapper>);
    juce::MidiMessageCollector& getMidiMessageCollector() noexcept { return messageCollector; }
private:
    juce::AudioProcessorGraph::NodeID midiIn;
    juce::AudioProcessorGraph::Node::Ptr begin, end;
    juce::MidiMessageCollector messageCollector;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
