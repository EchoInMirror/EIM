#pragma once

#include <packets.pb.h>
#include <juce_audio_processors/juce_audio_processors.h>

const std::string randomUuid();
int encodeMidiMessage(juce::MidiMessage& data);
juce::MidiMessage decodeMidiMessage(int data, double time);
juce::DynamicObject* savePluginState(juce::AudioPluginInstance* instance, juce::String id, juce::File& pluginsDir);
void runOnMainThread(std::function<void()> fn);
