#pragma once

#include "PluginWindow.h"
#include "Track.h"
#include "packets.pb.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack: public juce::AudioProcessorGraph, public juce::AudioPlayHead {
public:
    std::vector<juce::AudioProcessorGraph::Node::Ptr> tracks;
    std::unordered_map<std::string, juce::AudioProcessorGraph::Node::Ptr> tracksMap;
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    short ppq = 96;

    MasterTrack();
    ~MasterTrack() { deviceManager.closeAudioDevice(); }

    void removeTrack(std::string id);
    void stopAllNotes();
    juce::AudioProcessorGraph::Node::Ptr createTrack(std::string name, std::string color);
    void loadPlugin(std::unique_ptr<juce::PluginDescription> desc, juce::AudioPluginFormat::PluginCreationCallback callback);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void createPluginWindow(juce::AudioPluginInstance* instance);
    std::unique_ptr<EIMPackets::ProjectStatus> getProjectStatus();

    virtual bool getCurrentPosition(CurrentPositionInfo& result) override;
    virtual bool canControlTransport() override { return true; }
    virtual void transportPlay(bool shouldStartPlaying) override;
    virtual void transportRecord(bool shouldStartRecording) override { juce::ignoreUnused(shouldStartRecording); }
    virtual void transportRewind() override { }
private:
    int endTime = 0;
    juce::AudioProcessorGraph::NodeID outputNodeID;
    juce::AudioDeviceManager deviceManager;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    juce::AudioProcessorPlayer graphPlayer;
    std::unordered_map<juce::AudioPluginInstance*, PluginWindow> pluginWindows;

    void calcPositionInfo();
    juce::AudioProcessorGraph::Node::Ptr initTrack(std::unique_ptr<Track> track);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrack)
};
