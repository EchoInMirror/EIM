#include "Renderer.h"
#include "Main.h"

void Renderer::render(Renderable* target, std::unique_ptr<juce::AudioFormatWriter> output,
                      std::function<void()> callback) {
    this->renderTarget = target;
    this->output = std::move(output);
    this->callback = callback;
    DBG("start render");
    startTimer(1);
}

void Renderer::timerCallback() {
    if (this->renderTarget == nullptr || this->renderTarget->isRenderEnd()) {
        if (this->callback != nullptr) this->callback();
        stopTimer();
        DBG("end render");
        delete this->output.release();
        return;
    }
    rendering();
}

void Renderer::rendering() {
    assert(this->renderTarget != nullptr);
    juce::AudioBuffer<float> buffer(this->output->getNumChannels(), renderTarget->bufferBlockSize);
    this->renderTarget->processBlockBuffer(buffer);
    DBG("buffer : " << buffer.getNumChannels() << " ;" << buffer.getNumSamples());
    this->output->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}
