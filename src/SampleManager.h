#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class SampleManager {
public:
	struct SampleInfo {
		juce::String file;
		juce::String name;
		juce::String hash;
		double fullTime;
		double sampleRate;
		juce::AudioFormatReader* reader;
	};
	std::unordered_map<juce::String, SampleInfo> samples;

	SampleManager();
	SampleManager::SampleInfo* loadSample(juce::File);

	void drawWaveform(juce::AudioFormatReader* reader, juce::File file);
private:
	juce::PNGImageFormat pngFormat;
	juce::AudioFormatManager formatManager;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleManager)
};

class Sampler {
public:
	int startPPQ = 0, fullTime = 0;
	juce::AudioFormatReaderSource positionableSource;
	juce::ResamplingAudioSource resamplingAudioSource;
	SampleManager::SampleInfo* info;

	Sampler(const Sampler &);
	Sampler(SampleManager::SampleInfo*, int startPPQ = 0, int fullTime = 0);
};
