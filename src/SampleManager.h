#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

class SampleManager : public juce::ChangeListener {
public:
	struct Sample {
		juce::String file;
		juce::String hash;
		juce::AudioFormatReader* reader;
	};
	std::unordered_map<juce::String, Sample> samples;
	SampleManager();
	juce::String loadSample(juce::File);
	void drawWaveform();
	void changeListenerCallback(juce::ChangeBroadcaster* source) override;
private:
	juce::AudioFormatManager formatManager;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleManager)
};
