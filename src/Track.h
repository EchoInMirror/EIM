#pragma once

#include "utils/utils.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class MasterTrack;

class Track : public juce::AudioProcessorGraph {
  public:
    Track(std::string uuid);
    Track(std::string name, std::string color, std::string uuid = "");
	Track(juce::File trackRoot);
	Track(Track&& other);
	~Track();
    std::string uuid;
    std::string name;
    std::string color;
    juce::MidiMessageSequence midiSequence;
	float levelL = 0, levelR = 0;
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

    juce::AudioProcessorGraph::NodeID addEffectPlugin(std::unique_ptr<juce::AudioPluginInstance>);
    void removeEffectPlugin(juce::AudioPluginInstance*);
    void setInstrument(std::unique_ptr<juce::AudioPluginInstance>);
    juce::AudioPluginInstance* getInstrumentInstance();
    void addMidiEvents(juce::MidiMessageSequence seq, int timeFormat);
    void writeTrackInfo(EIMPackets::TrackInfo* data);
    void setMuted(bool val);
	void saveState();
	void saveState(juce::File);

  private:
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
