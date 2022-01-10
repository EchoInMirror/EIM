#pragma once

#include "PluginWrapper.h"
#include "SynchronizedAudioProcessorGraph.h"
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack;
class ByteBuffer;

class Track: public SynchronizedAudioProcessorGraph {
public:
    Track(std::string name, std::string color, MasterTrack* masterTrack);
    juce::Uuid uuid;
    std::string name;
    std::string color;

    juce::MidiMessageCollector messageCollector;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;
    void setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) override;

    void setGenerator(std::unique_ptr<PluginWrapper>);
    void addMidiEvents(juce::MidiMessageSequence seq, int timeFormat);
    void writeMidiData(ByteBuffer* buf);
private:
    MasterTrack* masterTrack;
    juce::CriticalSection processLock;
    int samplesPlayed = 0;
    double nextStartTime = 0;
    juce::AudioProcessorGraph::NodeID midiIn;
    juce::AudioProcessorGraph::Node::Ptr begin, end;
    juce::MidiMessageSequence midiSequence;

    void addMidiEventsToBuffer(int sampleCount, juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
