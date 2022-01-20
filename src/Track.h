#pragma once

#include "PluginWrapper.h"
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack;
class ByteBuffer;

class Track: public juce::AudioProcessorGraph {
public:
    Track(std::string name, std::string color, MasterTrack* masterTrack);
    juce::Uuid uuid;
    std::string name;
    std::string color;
    juce::MidiMessageSequence midiSequence;

    juce::AudioProcessorGraph::Node* currentNode = nullptr;
    juce::MidiMessageCollector messageCollector;
    juce::dsp::ProcessorChain<juce::dsp::Panner<float>, juce::dsp::Gain<float>> chain;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void setPlayHead(juce::AudioPlayHead* newPlayHead) override;
    virtual void setProcessingPrecision(ProcessingPrecision newPrecision);
    virtual void setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize);

    void setGenerator(std::unique_ptr<PluginWrapper>);
    void addMidiEvents(juce::MidiMessageSequence seq, int timeFormat);
    void writeTrackInfo(ByteBuffer* buf);
    void writeMidiData(ByteBuffer* buf);
    void setMuted(bool val);
private:
    MasterTrack* masterTrack;
    juce::CriticalSection processLock;
    int samplesPlayed = 0;
    double nextStartTime = 0;
    juce::AudioProcessorGraph::NodeID midiIn;
    juce::AudioProcessorGraph::Node::Ptr begin, end;

    void addMidiEventsToBuffer(int sampleCount, juce::MidiBuffer& midiMessages);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
