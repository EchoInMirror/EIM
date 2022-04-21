#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PluginWindow : public juce::DocumentWindow {
public:
    PluginWindow(juce::AudioPluginInstance* instance, std::unordered_map<juce::AudioPluginInstance*, PluginWindow>* pluginWindows);
    ~PluginWindow() {
		DBG("~PluginWindow");
		clearContentComponent();
	}

    void closeButtonPressed() override;
    int getDesktopWindowStyleFlags() const override;
private:
    bool resizable;
    juce::AudioPluginInstance* instance;
    std::unordered_map<juce::AudioPluginInstance*, PluginWindow>* pluginWindows;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
