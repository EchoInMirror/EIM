#include "SynchronizedAudioProcessorGraph.h"

void SynchronizedAudioProcessorGraph::setProcessingPrecision(ProcessingPrecision newPrecision) {
	AudioProcessorGraph::setProcessingPrecision(newPrecision);
	for (auto it : getNodes()) it->getProcessor()->setProcessingPrecision(newPrecision);
}

void SynchronizedAudioProcessorGraph::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) {
	AudioProcessorGraph::prepareToPlay(sampleRate, estimatedSamplesPerBlock);
	for (auto it : getNodes()) it->getProcessor()->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
}

void SynchronizedAudioProcessorGraph::setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) {
	AudioProcessorGraph::setRateAndBufferSizeDetails(newSampleRate, newBlockSize);
	for (auto it : getNodes()) it->getProcessor()->prepareToPlay(newSampleRate, newBlockSize);
}

void SynchronizedAudioProcessorGraph::setPlayHead(juce::AudioPlayHead* newPlayHead) {
	AudioProcessorGraph::setPlayHead(newPlayHead);
	for (auto it : getNodes()) it->getProcessor()->setPlayHead(newPlayHead);
}
