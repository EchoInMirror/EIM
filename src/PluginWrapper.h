#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>

class PluginWrapper: public juce::DocumentWindow, public juce::AudioProcessor {
public:
    PluginWrapper(std::unique_ptr<juce::AudioPluginInstance> instance);
    ~PluginWrapper() { clearContentComponent(); }
    float mixProportion = 1.0;

    void closeButtonPressed() override;
    int getDesktopWindowStyleFlags() const override;

    const juce::String getName() const override { return instance->getName(); }
    void releaseResources() override { instance->releaseResources(); }
    bool supportsDoublePrecisionProcessing() const override { return instance->supportsDoublePrecisionProcessing(); }
    void reset() override { instance->reset(); }
    void setNonRealtime(bool val) noexcept override { instance->setNonRealtime(val); }
    double getTailLengthSeconds() const override { return instance->getTailLengthSeconds(); }
    bool acceptsMidi() const override { return instance->acceptsMidi(); }
    bool producesMidi() const override { return instance->producesMidi(); }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return instance->getNumPrograms(); }
    int getCurrentProgram() override { return instance->getCurrentProgram(); }
    void setCurrentProgram(int val) override { instance->setCurrentProgram(val); }
    const juce::String getProgramName(int val) override { return instance->getProgramName(val); }
    void changeProgramName(int arg1, const juce::String& arg2) override { instance->changeProgramName(arg1, arg2); }
    void getStateInformation(juce::MemoryBlock& val) override { instance->getStateInformation(val); }
    void setStateInformation(const void* data, int sizeInBytes) override { instance->setStateInformation(data, sizeInBytes); }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) override;
    void setPlayHead(juce::AudioPlayHead* newPlayHead) override;
    virtual void setProcessingPrecision(ProcessingPrecision newPrecision);
    virtual void setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize);
private:
    juce::dsp::DryWetMixer<float> mixer;
    std::unique_ptr<juce::AudioPluginInstance> instance;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWrapper)
};
