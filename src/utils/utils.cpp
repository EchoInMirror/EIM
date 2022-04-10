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
    while (i < 10) arr[i++] = 32 + random.nextInt(96);
    arr[10] = 0;
	return arr;
}
