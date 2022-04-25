#pragma once
#include "Main.h"
#include "Renderable.h"
#include "packets.pb.h"
#include <juce_audio_utils/juce_audio_utils.h>

class Renderer : private juce::Timer {
public:
    Renderer(){};
    ~Renderer(){};
    void render(Renderable*, std::unique_ptr<juce::AudioFormatWriter>, std::function<void()> callback = nullptr);

private:
    Renderable* renderTarget = nullptr;
    std::unique_ptr<juce::AudioFormatWriter> output;
    std::function<void()> callback;
    void timerCallback() override;
    void rendering();
};
