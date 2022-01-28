#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class PluginManagerWindow : public juce::DocumentWindow {
public:
    PluginManagerWindow(juce::File rootPath);
    ~PluginManagerWindow() { pluginListComponent = nullptr; }

    bool isScanning = false;
    juce::AudioPluginFormatManager manager;
    juce::KnownPluginList knownPluginList;
    juce::PropertiesFile scanningProperties;

    void closeButtonPressed() override;
    void scanPlugins();
private:
    const juce::File knownPluginListXMLFile, deadMansPedalFile;
    std::unique_ptr<juce::PluginListComponent> pluginListComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManagerWindow)
};
