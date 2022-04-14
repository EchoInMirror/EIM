#include "utils.h"

juce::Random random;
const std::string randomUuid() {
    auto time = juce::Time::currentTimeMillis();
    char arr[11];
    int i = 0;
    while (time) {
        arr[i++] = 48 + (time & 63);
        time >>= 6;
    }
    while (i < 10) arr[i++] = 32 + (char)random.nextInt(96);
    arr[10] = 0;
	return arr;
}

int encodeMidiMessage(juce::MidiMessage& data) {
    auto raw = data.getRawData();
    int result = raw[0];
    switch (data.getRawDataSize()) {
    case 3: result |= raw[2] << 16;
    case 2: result |= raw[1] << 8;
    }
    return result;
}
