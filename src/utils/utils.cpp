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

void getPluginState(juce::AudioPluginInstance* instance, PluginState& state) {
	state.state.reset();
	instance->getStateInformation(state.state);
	state.name = instance->getPluginDescription().name;
	state.identifier = instance->getPluginDescription().createIdentifierString();
}

juce::DynamicObject* savePluginState(juce::AudioPluginInstance* instance, juce::String id, juce::File& pluginsDir) {
	juce::MemoryBlock memory;
	instance->getStateInformation(memory);
	auto xml = juce::AudioProcessor::getXmlFromBinary(memory.getData(), (int)memory.getSize());
	if (!xml) pluginsDir.getChildFile(id += ".bin").replaceWithData(memory.getData(), memory.getSize());
	else {
		auto file = pluginsDir.getChildFile(id += ".xml");
		file.deleteFile();
		xml.release()->writeTo(file);
	}
	auto obj = new juce::DynamicObject();
	obj->setProperty("identifier", instance->getPluginDescription().createIdentifierString());
	obj->setProperty("stateFile", id);
	return obj;
}

class AsyncFunctionCallback : public juce::MessageManager::MessageBase {
public:
	AsyncFunctionCallback(std::function<void()> const f) : func(f) {}

	void messageCallback() override {
		func();
		finished.signal();
	}

	juce::WaitableEvent finished;
private:
	std::function<void()> const func;

	JUCE_DECLARE_NON_COPYABLE(AsyncFunctionCallback)
};

void runOnMainThread(std::function<void()> fn) {
	auto instance = juce::MessageManager::getInstance();
	if (instance->isThisTheMessageThread()) return fn();

	// If this thread has the message manager locked, then this will deadlock!
	jassert(!instance->currentThreadHasLockedMessageManager());

	const juce::ReferenceCountedObjectPtr<AsyncFunctionCallback> message(new AsyncFunctionCallback(fn));

	if (message->post()) message->finished.wait();
}

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>
#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#endif
#endif

size_t getCurrentRSS() {
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
		(task_info_t)&info, &infoCount) != KERN_SUCCESS)
		return (size_t)0L;      /* Can't access? */
	return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ((fp = fopen("/proc/self/statm", "r")) == NULL)
		return (size_t)0L;      /* Can't open? */
	if (fscanf(fp, "%*s%ld", &rss) != 1)
	{
		fclose(fp);
		return (size_t)0L;      /* Can't read? */
	}
	fclose(fp);
	return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;          /* Unsupported. */
#endif
}
