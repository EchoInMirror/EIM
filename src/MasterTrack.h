#pragma once

#include "Track.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack: public juce::AudioProcessorGraph, public juce::AudioPlayHead {
public:
    std::vector<juce::AudioProcessorGraph::Node::Ptr> tracks;
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    double endTime = 0;
    short ppq = 96;

    using PluginCreationCallback = std::function<void(std::unique_ptr<PluginWrapper>, const std::string&)>;

    MasterTrack();
    ~MasterTrack() { deviceManager.closeAudioDevice(); }

    void removeTrack(int id);
    void stopAllNotes();
    juce::AudioProcessorGraph::Node::Ptr createTrack(std::string name, std::string color);
    void loadPlugin(std::unique_ptr<juce::PluginDescription> desc, PluginCreationCallback callback);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    virtual bool getCurrentPosition(CurrentPositionInfo& result) override;
    virtual bool canControlTransport() override { return true; }
    virtual void transportPlay(bool shouldStartPlaying) override;
    virtual void transportRecord(bool shouldStartRecording) override { juce::ignoreUnused(shouldStartRecording); }
    virtual void transportRewind() override { }
private:
    juce::AudioProcessorGraph::NodeID outputNodeID;
    juce::AudioDeviceManager deviceManager;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    juce::AudioProcessorPlayer graphPlayer;

    void calcPositionInfo();
    juce::AudioProcessorGraph::Node::Ptr initTrack(std::unique_ptr<Track> track);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrack)
};
