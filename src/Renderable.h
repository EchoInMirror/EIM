#pragma once
#include "packets.pb.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

class Renderer;
class Renderable {
	friend class Renderer;
public:
    Renderable(){};
    ~Renderable(){};
	virtual void renderStart(double, int) { };
	virtual void renderEnd() = 0;
    virtual float getProgress() = 0;
    virtual void processBlockBuffer(juce::AudioBuffer<float>&) = 0;
    virtual void render(juce::File, std::unique_ptr<EIMPackets::ServerboundRender>) = 0;
	bool isRendering() { return _isRendering; }

private:
	bool _isRendering = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Renderable)
};
