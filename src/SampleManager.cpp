#include "SampleManager.h"
#include "juce_cryptography/hashing/juce_MD5.h"
#include <juce_audio_devices/sources/juce_AudioTransportSource.h>

SampleManager::SampleManager() {
	formatManager.registerBasicFormats();
}

juce::String SampleManager::loadSample(juce::File file) {
	auto key = file.getFileName();
	/*
	juce::SamplerVoice a;
	if (samples.contains(key)) return key;
	formatManager.getDefaultFormat()->createMemoryMappedReader();
	auto reader = formatManager.createReaderFor(file);
	//reader->
	samples[key] = { file.getFullPathName(), juce::MD5(file).toHexString(),  };
	juce::MemoryMappedAudioFormatReader gg(file, reader, );
	if (samples.)
	juce::AudioTransportSource t;
	juce::AudioFormatReaderSource a(reader, false);
	t.setSource(&a, 0, nullptr, reader->sampleRate);
	*/
	return key;
}

void SampleManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
	/*
	if (source == &thumbnail && thumbnail.isFullyLoaded()) {
		auto length = thumbnail.getTotalLength();
		int width = juce::roundToInt(length / 60.0 * currentPositionInfo.bpm * 96 * 6);
		juce::Rectangle<int> thumbnailBounds(0, 0, width, 70);
		auto img = juce::Image(juce::Image::ARGB, width, 70, true);
		juce::Graphics g(img);
		g.setColour(juce::Colours::white);
		thumbnail.drawChannels(g, thumbnailBounds, 0.0, length, 1.0f);
		juce::PNGImageFormat format;
		auto file = juce::File::getCurrentWorkingDirectory().getChildFile("test.png");
		file.deleteFile();
		juce::FileOutputStream stream(file);
		format.writeImageToStream(img, stream);
		DBG("FINISHED");
	}
	*/
}

void SampleManager::drawWaveform() {
	/*
	auto topY = (float)area.getY();
	auto bottomY = (float)area.getBottom();
	auto midY = (topY + bottomY) * 0.5f;
	auto vscale = verticalZoomFactor * (bottomY - topY) / 256.0f;

	auto* cacheData = getData(channelNum, clip.getX() - area.getX());

	RectangleList<float> waveform;
	waveform.ensureStorageAllocated(clip.getWidth());

	auto x = (float)clip.getX();

	for (int w = clip.getWidth(); --w >= 0;)
	{
		if (cacheData->isNonZero())
		{
			auto top = jmax(midY - cacheData->getMaxValue() * vscale - 0.3f, topY);
			auto bottom = jmin(midY - cacheData->getMinValue() * vscale + 0.3f, bottomY);

			waveform.addWithoutMerging(Rectangle<float>(x, top, 1.0f, bottom - top));
		}

		x += 1.0f;
		++cacheData;
	}

	g.fillRectList(waveform);
	*/
}
