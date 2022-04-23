#pragma once
#include "Renderable.h"
#include <juce_audio_utils/juce_audio_utils.h>

class RenderParameter {
public:
    RenderParameter();
    double sampleRateToUse;
    unsigned int numberOfChannels;
    int bitsPerSample;
};

class Renderer : private juce::Timer {
public:
    Renderer(){};
    ~Renderer(){};
    void render(Renderable*, std::unique_ptr<juce::AudioFormatWriter>);

private:
    Renderable* renderTarget = nullptr;
    std::unique_ptr<juce::AudioFormatWriter> output;
    void timerCallback() override;
    void rendering();
};
