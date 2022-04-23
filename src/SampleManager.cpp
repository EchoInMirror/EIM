#include "SampleManager.h"
#include "Main.h"
#include "juce_cryptography/hashing/juce_MD5.h"
#define EIM_WAVEFORM_HEIGHT 140

SampleManager::SampleManager() {
	formatManager.registerBasicFormats();
}

SampleManager::SampleInfo* SampleManager::loadSample(juce::File file) {
	auto key = file.getFileName();
	juce::SamplerVoice a;
	if (samples.contains(key)) return &samples[key];
	if (!file.existsAsFile()) return nullptr;
	auto reader = formatManager.createReaderFor(file);
	double sampleRate = reader->sampleRate;
	auto instance = EIMApplication::getEIMInstance();
	auto res = &(samples[key] = {
		file.getFullPathName(),
		key,
		juce::MD5(file).toHexString(),
		reader->lengthInSamples / sampleRate,
		sampleRate,
		reader
	});
	auto image = instance->config.projectSamplesPreviewPath.getChildFile(key + ".png");
	if (!image.existsAsFile()) drawWaveform(reader, image);
	return res;
}

void SampleManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
	/*
	if (source == &thumbnail && thumbnail.isFullyLoaded()) {
		auto length = thumbnail.getTotalLength();
		int width = 
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

void SampleManager::drawWaveform(juce::AudioFormatReader* reader, juce::File file) {
	auto sampleRate = reader->sampleRate;
	if (sampleRate <= 0) return;
	std::thread thread([this, reader, file, sampleRate] {
		auto channels = juce::jmin(2, (int)reader->numChannels);
		auto isTwoChannels = channels == 2;
		int length = juce::roundToInt(reader->lengthInSamples);
		int width = juce::roundToInt(reader->lengthInSamples / sampleRate * 1000.0);
		int step = length / width;
		float l = 0;
		float r = 0;
		float* const w[] = { &l, &r };
		auto img = juce::Image(juce::Image::ARGB, width, EIM_WAVEFORM_HEIGHT, true);
		juce::Graphics g(img);
		g.setColour(juce::Colours::white);
		int halfHieght = EIM_WAVEFORM_HEIGHT / 2;
		int quarterHieght = EIM_WAVEFORM_HEIGHT / 4;
		int threeFourthsHeight = halfHieght + quarterHieght;
		for (int i = 0; i < width; i++) {
			reader->read(w, channels, (juce::int64)i * step, 1);
			if (isTwoChannels) {
				int tl = (int)(quarterHieght * juce::jlimit(-1.0f, 1.0f, l)),
					tr = (int)(quarterHieght * juce::jlimit(-1.0f, 1.0f, r));
				if (tl > 0) g.fillRect(i, quarterHieght - tl, 1, tl);
				else g.fillRect(i, quarterHieght, 1, -tl);
				if (tr > 0) g.fillRect(i, threeFourthsHeight - tr, 1, tr);
				else g.fillRect(i, threeFourthsHeight, 1, -tr);
			}
			else {
				int tl = (int)(halfHieght * juce::jlimit(-1.0f, 1.0f, l));
				if (tl > 0) g.fillRect(i, halfHieght - tl, 1, tl);
				else g.fillRect(i, halfHieght, 1, -tl);
			}
		}
		juce::FileOutputStream stream(file);
		pngFormat.writeImageToStream(img, stream);
	});
	thread.detach();
}

Sampler::Sampler(const Sampler& other) : info(other.info), startPPQ(other.startPPQ),
	positionableSource(other.info->reader, false),
	resamplingAudioSource(&positionableSource, false, juce::jmin((int)other.info->reader->numChannels, 2)) {
	resamplingAudioSource.setResamplingRatio(other.info->sampleRate / EIMApplication::getEIMInstance()->mainWindow->masterTrack->getSampleRate());
}
Sampler::Sampler(SampleManager::SampleInfo* info, int startPPQ) : info(info), startPPQ(startPPQ),
	positionableSource(info->reader, false),
	resamplingAudioSource(&positionableSource, false, juce::jmin((int)info->reader->numChannels, 2)) {
	resamplingAudioSource.setResamplingRatio(info->sampleRate / EIMApplication::getEIMInstance()->mainWindow->masterTrack->getSampleRate());
}
