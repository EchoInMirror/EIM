#pragma once

#include "PluginWindow.h"
#include <juce_audio_utils/juce_audio_utils.h>

class PluginManager : juce::Timer {
  public:
    PluginManager(juce::File rootPath);
	~PluginManager();

    juce::AudioPluginFormatManager manager;
    juce::KnownPluginList knownPluginList;
    std::unordered_map<juce::AudioPluginInstance*, PluginWindow> pluginWindows;

    void scanPlugins();
    void stopScanning();
    void timerCallback() override;
    bool isScanning();
	void skipScanning(int);
    void createPluginWindow(juce::AudioPluginInstance* instance);

  private:
    int numThreads = 10, numFiles = 0;
    bool _isScanning = false;
    bool inited = false;
    const juce::File knownPluginListXMLFile;
    juce::String scannerPath;
    std::queue<juce::String> scainngFiles;
    juce::ChildProcess* processes = nullptr;
    std::vector<juce::String> processScanFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};
