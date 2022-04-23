#include "Renderer.h"
#include "Main.h"

void Renderer::render(Renderable* target, std::unique_ptr<juce::AudioFormatWriter> output) {
    this->renderTarget = target;
    this->output = std::move(output);
    DBG("start render");
    startTimer(1);
}

void Renderer::timerCallback() {
    if (this->renderTarget == nullptr || this->renderTarget->isRenderEnd()) {
        stopTimer();
        DBG("end render");
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