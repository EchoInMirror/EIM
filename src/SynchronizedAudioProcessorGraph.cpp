#include "SynchronizedAudioProcessorGraph.h"

void SynchronizedAudioProcessorGraph::setProcessingPrecision(ProcessingPrecision newPrecision) {
	AudioProcessorGraph::setProcessingPrecision(newPrecision);
	auto nodes = getNodes();
	for (auto it = nodes.begin(); it != nodes.end(); it++) (*it)->getProcessor()->setProcessingPrecision(newPrecision);
}

void SynchronizedAudioProcessorGraph::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) {
	AudioProcessorGraph::prepareToPlay(sampleRate, estimatedSamplesPerBlock);
	auto nodes = getNodes();
	for (auto it = nodes.begin(); it != nodes.end(); it++) (*it)->getProcessor()->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
}

void SynchronizedAudioProcessorGraph::setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) {
	AudioProcessorGraph::setRateAndBufferSizeDetails(newSampleRate, newBlockSize);
	auto nodes = getNodes();
	for (auto it = nodes.begin(); it != nodes.end(); it++) (*it)->getProcessor()->prepareToPlay(newSampleRate, newBlockSize);
}
