#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class PluginManager : juce::Timer {
  public:
    PluginManager(juce::File rootPath);

    juce::AudioPluginFormatManager manager;
    juce::KnownPluginList knownPluginList;

    void scanPlugins();
    void stopScanning();
    void timerCallback() override;
    bool isScanning();

  private:
    int numThreads = 10;
    bool _isScanning = false;
    bool inited = false;
    const juce::File knownPluginListXMLFile;
    juce::String scannerPath;
    std::queue<juce::String> scainngFiles;
    juce::ChildProcess* processes = nullptr;
    std::vector<juce::String> processScanFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};
