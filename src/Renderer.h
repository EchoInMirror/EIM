#pragma once
#include "Renderable.h"
#include "packets.pb.h"
#include <juce_audio_utils/juce_audio_utils.h>

class Renderer : private juce::Timer {
public:
    Renderer(Renderable*, std::unique_ptr<juce::AudioFormatWriter>, std::function<void()> = nullptr);
    ~Renderer() {};
    void render();

private:
    int lastSendTime = 0;
    Renderable* renderTarget = nullptr;
	juce::AudioBuffer<float> buffer;
    std::unique_ptr<juce::AudioFormatWriter> output;
    std::function<void()> callback;
	EIMPackets::ClientboundRenderProgress progress;
    void timerCallback() override;
};
