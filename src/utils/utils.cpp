#include "utils.h"

const std::string uuidToString(juce::Uuid& uuid) {
	return reinterpret_cast<const char*>(uuid.getRawData());
}
