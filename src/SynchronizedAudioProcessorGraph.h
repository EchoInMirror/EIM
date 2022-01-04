#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class SynchronizedAudioProcessorGraph : public juce::AudioProcessorGraph {
public:
    SynchronizedAudioProcessorGraph(): juce::AudioProcessorGraph() {}
    virtual void setProcessingPrecision(ProcessingPrecision newPrecision);
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    virtual void setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize);
private:
    juce::AudioProcessorGraph::Node::Ptr begin, end;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynchronizedAudioProcessorGraph)
};
