#pragma once

#include <packets.pb.h>
#include <juce_audio_basics/juce_audio_basics.h>

const std::string randomUuid();
int encodeMidiMessage(juce::MidiMessage& data);