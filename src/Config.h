#include <juce_audio_utils/juce_audio_utils.h>

class Config {
public:
    Config();

	bool changed = false;
    juce::var config;
    const juce::File rootPath, configPath;

    std::string toString();
	void save();
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Config)
};
