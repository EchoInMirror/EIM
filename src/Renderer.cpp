#include "Renderer.h"
#include "packets.h"
#include "Main.h"

Renderer::Renderer(Renderable* target, std::unique_ptr<juce::AudioFormatWriter> output,
	std::function<void()> callback) : renderTarget(target), output(std::move(output)), callback(callback), buffer(2, 2048) {
}

void Renderer::render() {
	DBG("start render");
	renderTarget->_isRendering = true;
	renderTarget->renderStart(output->getSampleRate(), 2048);
	startTimer(1);
}

void Renderer::timerCallback() {
	auto prog = renderTarget->getProgress();
    if (prog >= 0) {
		stopTimer();
		output.reset(nullptr);
		renderTarget->_isRendering = false;
		if (callback) callback();
		progress.set_progress(1);
		EIMApplication::getEIMInstance()->listener->boardcast(
			std::move(EIMMakePackets::makeRenderProgressPacket(progress)));
		renderTarget->renderEnd();
        DBG("end render");
        return;
    }
    if (++lastSendTime >= 500) {
        lastSendTime = 0;
        progress.set_progress(prog);
		EIMApplication::getEIMInstance()->listener->boardcast(
			std::move(EIMMakePackets::makeRenderProgressPacket(progress)));
    }
	buffer.clear();
	renderTarget->processBlockBuffer(buffer);
	output->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}
