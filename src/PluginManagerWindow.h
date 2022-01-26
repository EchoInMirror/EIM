#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class PluginManagerWindow : public juce::DocumentWindow {
public:
    PluginManagerWindow(juce::File rootPath);
    ~PluginManagerWindow() { pluginListComponent = nullptr; }
    juce::AudioPluginFormatManager manager;
    juce::KnownPluginList knownPluginList;

    std::unique_ptr<juce::PluginListComponent> pluginListComponent;
    void closeButtonPressed() override;
private:
    const juce::File knownPluginListXMLFile, deadMansPedalFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerWindow)
};
