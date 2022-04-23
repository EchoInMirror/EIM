#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class Renderable {
public:
    Renderable(){};
    ~Renderable(){};
    int bufferBlockSize = 0;
    virtual bool isRenderEnd() = 0;
    virtual void processBlockBuffer(juce::AudioBuffer<float>&) = 0;
    virtual void render(juce::File) = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Renderable)
};
