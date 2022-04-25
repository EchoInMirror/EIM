#pragma once

#include "Renderable.h"
#include "Renderer.h"
#include "Track.h"
#include "packets.pb.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

class MasterTrack : public juce::AudioProcessorGraph,
                    public juce::AudioPlayHead,
                    public juce::ChangeListener,
                    public Renderable,
                    private juce::Timer {
public:
    std::vector<juce::AudioProcessorGraph::Node::Ptr> tracks;
    std::unordered_map<std::string, juce::AudioProcessorGraph::Node::Ptr> tracksMap;
    juce::AudioPlayHead::CurrentPositionInfo currentPositionInfo;
    SampleManager sampleManager;
    short ppq = 96;
    int events = 0;
    int projectTime = 0;
    EIMPackets::ClientboundPong systemInfo;

    MasterTrack();
    ~MasterTrack() {
        deviceManager.closeAudioDevice();
    }

    void removeTrack(std::string id);
    void stopAllNotes();
    juce::AudioProcessorGraph::Node::Ptr addTrack(std::unique_ptr<Track> track);
    void loadPlugin(PluginState& state, juce::AudioPluginFormat::PluginCreationCallback callback);
    void loadPlugin(std::unique_ptr<juce::PluginDescription> desc,
                    juce::AudioPluginFormat::PluginCreationCallback callback);
    void loadPluginFromFile(juce::var& json, juce::File data, juce::AudioPluginFormat::PluginCreationCallback callback);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void writeProjectStatus(EIMPackets::ProjectStatus&);
    void init();
    void saveState();
    void loadProject(juce::File);
    void checkEndTime();
    void checkEndTime(int endTime);

    bool isRenderEnd() override;
    void processBlockBuffer(juce::AudioBuffer<float>&) override;
    void render(juce::File, std::unique_ptr<EIMPackets::ServerboundRender>) override;

    bool getCurrentPosition(CurrentPositionInfo& result) override;
    bool canControlTransport() override {
        return true;
    }
    void transportPlay(bool shouldStartPlaying) override;
    void transportRecord(bool shouldStartRecording) override {
        juce::ignoreUnused(shouldStartRecording);
    }
    void transportRewind() override {
    }

private:
    int endTime = 0;
    juce::AudioProcessorGraph::NodeID outputNodeID;
    juce::AudioDeviceManager deviceManager;
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    juce::AudioProcessorPlayer graphPlayer;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<std::string> deletedTracks;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<juce::FileOutputStream> outStream;
    std::unique_ptr<juce::AudioFormatWriter> audioWirte;
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
