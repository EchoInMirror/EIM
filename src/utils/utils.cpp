#include "utils.h"

char chars[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'B', 'C', 'D',
	'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U',
	'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
	'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '-', '_' };

juce::Random random;
const std::string randomUuid() {
    auto time = juce::Time::currentTimeMillis();
    char arr[11] = { };
    int i = 0;
    while (time) {
        arr[i++] = chars[time & 63];
        time >>= 6;
    }
    while (i < 10) arr[i++] = chars[random.nextInt(64)];
    arr[10] = 0;
	return arr;
}

int encodeMidiMessage(juce::MidiMessage& data) {
    auto raw = data.getRawData();
    int result = raw[0];
    switch (data.getRawDataSize()) {
    case 3:
		result |= raw[2] << 16;
		[[fallthrough]];
    case 2: result |= raw[1] << 8;
    }
    return result;
}

juce::MidiMessage decodeMidiMessage(int data, double time) {
    int b1 = data & 0xff, b2 = (data >> 8) & 0xff;
    switch (juce::MidiMessage::getMessageLengthFromFirstByte((juce::uint8)b1)) {
    case 1: return juce::MidiMessage(b1, time);
    case 2: return juce::MidiMessage(b1, b2, time);
    default: return juce::MidiMessage(b1, b2, (data >> 16) & 0xff, time);
    }
}
