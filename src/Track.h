#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "utils/utils.h"

class MasterTrack;
class ByteBuffer;

class Track: public juce::AudioProcessorGraph {
public:
    Track(std::string uuid, MasterTrack* masterTrack);
    Track(std::string name, std::string color, MasterTrack* masterTrack);
    std::string uuid;
    std::string name;
    std::string color;
    juce::MidiMessageSequence midiSequence;
    int pan = 0;

    juce::AudioProcessorGraph::Node* currentNode = nullptr;
    juce::MidiMessageCollector messageCollector;
    juce::dsp::ProcessorChain<juce::dsp::Panner<float>, juce::dsp::Gain<float>> chain;
    std::vector<juce::AudioProcessorGraph::Node::Ptr> plugins;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void setPlayHead(juce::AudioPlayHead* newPlayHead) override;
    virtual void setProcessingPrecision(ProcessingPrecision newPrecision);
    virtual void setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize);

    void addEffectPlugin(std::unique_ptr<juce::AudioPluginInstance>);
    void setInstrument(std::unique_ptr<juce::AudioPluginInstance>);
    juce::AudioPluginInstance* getInstrumentInstance();
    void addMidiEvents(juce::MidiMessageSequence seq, int timeFormat);
    EIMPackets::TrackInfo getTrackInfo();
    void writeMidiData(ByteBuffer* buf);
    void syncThisTrackMixerInfo();
    void setMuted(bool val);
private:
    MasterTrack* masterTrack;
    juce::CriticalSection processLock;
    int samplesPlayed = 0;
    double nextStartTime = 0;
    juce::AudioProcessorGraph::NodeID midiIn, begin, end;
    juce::AudioProcessorGraph::Node::Ptr instrumentNode = nullptr;
    juce::dsp::PannerRule panRule = juce::dsp::PannerRule::balanced;

    void init();
    void addMidiEventsToBuffer(int sampleCount, juce::MidiBuffer& midiMessages);
    void addAudioConnection(juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
