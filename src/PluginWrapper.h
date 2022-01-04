#pragma once

#include "SynchronizedAudioProcessorGraph.h"

class PluginWrapper: public juce::DocumentWindow, public SynchronizedAudioProcessorGraph {
public:
    PluginWrapper(std::unique_ptr<juce::AudioPluginInstance> instance);
    ~PluginWrapper() {
        clearContentComponent();
    }

    void closeButtonPressed() override;
    int getDesktopWindowStyleFlags() const override;
private:
    juce::AudioProcessorGraph::Node::Ptr pluginNode;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWrapper)
};
