#include "PluginWrapper.h"

PluginWrapper::PluginWrapper(std::unique_ptr<juce::AudioPluginInstance> p) :
	juce::DocumentWindow(p->getName(), juce::Colours::white, juce::DocumentWindow::closeButton),
	AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)),
	instance(std::move(p)) {
	setContentOwned(instance->createEditorIfNeeded(), true);
	mixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);
	setUsingNativeTitleBar(true);
	setResizable(true, true);
	setVisible(true);
}

int PluginWrapper::getDesktopWindowStyleFlags() const {
	return DocumentWindow::getDesktopWindowStyleFlags();
}

void PluginWrapper::closeButtonPressed() {
	setVisible(false);
}

void PluginWrapper::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	mixer.pushDrySamples(buffer);
	instance->processBlock(buffer, midiMessages);
	mixer.mixWetSamples(buffer);
}

void PluginWrapper::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	mixer.pushDrySamples(buffer);
	instance->processBlockBypassed(buffer, midiMessages);
	mixer.mixWetSamples(buffer);
}

void PluginWrapper::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) {
	const auto channels = juce::jmax(instance->getTotalNumInputChannels(), instance->getTotalNumOutputChannels());
	mixer.prepare({ sampleRate, (juce::uint32)estimatedSamplesPerBlock, (juce::uint32)channels });
	instance->setRateAndBufferSizeDetails(sampleRate, estimatedSamplesPerBlock);
	instance->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
}
void PluginWrapper::setPlayHead(juce::AudioPlayHead* newPlayHead) {
	instance->setPlayHead(newPlayHead);
}
void PluginWrapper::setProcessingPrecision(ProcessingPrecision newPrecision) {
	instance->setProcessingPrecision(newPrecision);
}
void PluginWrapper::setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) {
	instance->prepareToPlay(newSampleRate, newBlockSize);
}
