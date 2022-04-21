#pragma once

#include "Track.h"
#include "packets.pb.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack: public juce::AudioProcessorGraph, public juce::AudioPlayHead, public juce::ChangeListener,
	private juce::Timer {
public:
    std::vector<juce::AudioProcessorGraph::Node::Ptr> tracks;
    std::unordered_map<std::string, juce::AudioProcessorGraph::Node::Ptr> tracksMap;
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    short ppq = 96;
	int events = 0;
	int projectTime = 0;
	EIMPackets::ClientboundPong systemInfo;

    MasterTrack();
    ~MasterTrack() { deviceManager.closeAudioDevice(); }

    void removeTrack(std::string id);
    void stopAllNotes();
	juce::AudioProcessorGraph::Node::Ptr addTrack(std::unique_ptr<Track> track);
	void loadPlugin(PluginState& state, juce::AudioPluginFormat::PluginCreationCallback callback);
    void loadPlugin(std::unique_ptr<juce::PluginDescription> desc, juce::AudioPluginFormat::PluginCreationCallback callback);
	void loadPluginFromFile(juce::var& json, juce::File data, juce::AudioPluginFormat::PluginCreationCallback callback);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
	void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void writeProjectStatus(EIMPackets::ProjectStatus&);
	void init();
	void saveState();
	void loadProject(juce::File);
	void checkEndTime();
	void checkEndTime(int endTime);

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
	juce::AudioFormatManager formatManager;
	juce::AudioThumbnailCache thumbnailCache;
	juce::AudioThumbnail thumbnail;
	std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
	std::unique_ptr<juce::FileChooser> chooser;
	std::vector<std::string> deletedTracks;

    void calcPositionInfo();
	void timerCallback() override;

	class SystemInfoTimer : public juce::Timer {
	public:
		SystemInfoTimer();
		void timerCallback() override;
	private:
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SystemInfoTimer)
	};
	SystemInfoTimer systemInfoTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrack)
};
