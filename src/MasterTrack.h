#pragma once

#include "Track.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack: private SynchronizedAudioProcessorGraph, private juce::Timer {
public:
    MasterTrack();
    ~MasterTrack() {
        deviceManager.closeAudioDevice();
    }

    void scanPlugins();
    void removeTrack(int id);
    void timerCallback() override;
    juce::AudioProcessorGraph::Node::Ptr createTrack();
    std::unique_ptr<PluginWrapper> loadPlugin(int id);
private:
    double startTime = 0;
    int sampleRate = 96000, bufferSize = 1024;
    juce::File knownPluginListXMLFile;
    std::vector<juce::AudioProcessorGraph::Node::Ptr> tracks;
    juce::KnownPluginList list;
    juce::AudioProcessorGraph::NodeID outputNodeID;
    juce::AudioPluginFormatManager manager;
    juce::AudioDeviceManager deviceManager;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    juce::AudioProcessorPlayer graphPlayer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrack)
};
