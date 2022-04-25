#include "Renderer.h"
#include "packets.h"
#include "Main.h"

Renderer::Renderer(Renderable* target, std::unique_ptr<juce::AudioFormatWriter> output,
	std::function<void()> callback) : renderTarget(target), output(std::move(output)), callback(callback) {
}

void Renderer::render() {
	DBG("start render");
	startTimer(1);
}

void Renderer::timerCallback() {
    if (renderTarget == nullptr || renderTarget->isRenderEnd()) {
        if (callback != nullptr) callback();
        stopTimer();
		delete output.release();
        DBG("end render");
		renderTarget->renderEnd();
        return;
    }
    lastSendTime += getTimerInterval();
    if (lastSendTime >= 500) {
        lastSendTime = 0;
        EIMPackets::ClientboundRenderProgress progress;
        progress.set_progress(renderTarget->getProgress());
        EIMApplication::getEIMInstance()->listener->boardcast(
            std::move(EIMMakePackets::makeRenderProgressPacket(progress)));
    }
    rendering();
}

void Renderer::rendering() {
    assert(renderTarget != nullptr);
    assert(output != nullptr);
    juce::AudioBuffer<float> buffer(output->getNumChannels(), renderTarget->bufferBlockSize);
    renderTarget->processBlockBuffer(buffer);
    output->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}
