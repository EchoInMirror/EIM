#include "Track.h"
#include "Main.h"
#include "MasterTrack.h"
#include "ProcessorBase.h"

Track::Track(std::string id, MasterTrack* masterTrack) : uuid(id), AudioProcessorGraph(), masterTrack(masterTrack) {
    init();
}

Track::Track(std::string name, std::string color, MasterTrack* masterTrack, std::string uuid)
    : uuid(uuid.empty() ? randomUuid() : uuid), AudioProcessorGraph(), name(name), color(color), masterTrack(masterTrack) {
    init();
}

void Track::init() {
    setChannelLayoutOfBus(true, 0, juce::AudioChannelSet::canonicalChannelSet(2));
    setChannelLayoutOfBus(false, 0, juce::AudioChannelSet::canonicalChannelSet(2));
    auto input = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    begin = addNode(std::make_unique<ProcessorBase>())->nodeID;
    end = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                      juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))
              ->nodeID;
    midiIn = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                         juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode))
                 ->nodeID;
    auto midiOut = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                               juce::AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode))
                       ->nodeID;

    addAudioConnection(input->nodeID, begin);
    addAudioConnection(begin, end);
    addConnection({{midiIn, juce::AudioProcessorGraph::midiChannelIndex},
                   {midiOut, juce::AudioProcessorGraph::midiChannelIndex}});
    chain.get<1>().setGainLinear(1);
}

juce::AudioProcessorGraph::NodeID Track::addEffectPlugin(std::unique_ptr<juce::AudioPluginInstance> plugin) {
    auto node = plugins.emplace_back(addNode(std::move(plugin)))->nodeID;
    juce::AudioProcessorGraph::NodeID prev;
    for (auto& conn : getConnections())
        if (conn.destination.nodeID == end) {
            prev = conn.source.nodeID;
            removeConnection(conn);
        }
    addAudioConnection(prev, node);
    addAudioConnection(node, end);
    return node;
}

void Track::removeEffectPlugin(juce::AudioPluginInstance* instance) {
    for (auto node : getNodes()) {
        if (node->getProcessor() == instance) removeNode(node);
    }
}

void Track::setInstrument(std::unique_ptr<juce::AudioPluginInstance> instance) {
    if (instance == nullptr) {
        instrumentNode = nullptr;
        return;
    }
    instrumentNode = addNode(std::move(instance));
    addAudioConnection(instrumentNode->nodeID, begin);
    addConnection({{midiIn, juce::AudioProcessorGraph::midiChannelIndex},
                   {instrumentNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
}

void Track::setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) {
    AudioProcessorGraph::setRateAndBufferSizeDetails(newSampleRate, newBlockSize);
    for (auto it : getNodes())
        it->getProcessor()->prepareToPlay(newSampleRate, newBlockSize);
    messageCollector.reset(newSampleRate);
}

void Track::addAudioConnection(juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest) {
    addConnection({{src, 0}, {dest, 0}});
    addConnection({{src, 1}, {dest, 1}});
}

void Track::addMidiEventsToBuffer(int sampleCount, juce::MidiBuffer& midiMessages) {
    auto& info = masterTrack->currentPositionInfo;
    if (info.isPlaying) {
        auto startTime = info.ppqPosition;
        auto totalTime = sampleCount / getSampleRate() / 60.0 * info.bpm * masterTrack->ppq;
        auto endTime = startTime + totalTime;
        double curTime;
        for (auto it = midiSequence.begin() + midiSequence.getNextIndexAtTime(startTime);
             it < midiSequence.end() && (curTime = (*it)->message.getTimeStamp()) < endTime; it++) {
            midiMessages.addEvent((*it)->message, juce::roundToInt((curTime - startTime) / totalTime * sampleCount));
        }
    }
    messageCollector.removeNextBlockOfMessages(midiMessages, sampleCount);
}

void Track::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    addMidiEventsToBuffer(buffer.getNumSamples(), midiMessages);
    AudioProcessorGraph::processBlock(buffer, midiMessages);
    auto inoutBlock = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, (size_t)2);
    chain.process(juce::dsp::ProcessContextReplacing<float>(inoutBlock));
}

void Track::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) {
    addMidiEventsToBuffer(buffer.getNumSamples(), midiMessages);
    AudioProcessorGraph::processBlock(buffer, midiMessages);
}

void Track::addMidiEvents(juce::MidiMessageSequence seq, int timeFormat) {
    for (auto node : seq) {
        if (!node->message.isNoteOnOrOff()) continue;
        juce::MidiMessage msg = node->message;
        msg.setTimeStamp(juce::roundToInt(msg.getTimeStamp() / timeFormat * masterTrack->ppq));
        midiSequence.addEvent(msg, 0);
    }
}

void Track::writeTrackInfo(EIMPackets::TrackInfo* data) {
    data->set_uuid(uuid);
    data->set_name(name);
    data->set_color(color);
    data->set_muted(currentNode->isBypassed());
    data->set_solo(false);
    data->set_hasinstrument(instrumentNode != nullptr);
    data->set_volume(chain.get<1>().getGainLinear());
    data->set_pan(pan);
	data->set_isreplacing(true);
    for (auto& it : plugins) {
        data->add_plugins()->set_name(it->getProcessor()->getName().toStdString());
    }
    for (auto& it : midiSequence) {
        auto note = data->add_midi();
        note->set_time((int)it->message.getTimeStamp());
        note->set_data(encodeMidiMessage(it->message));
    }
}

juce::AudioPluginInstance* Track::getInstrumentInstance() {
    return instrumentNode == nullptr ? nullptr : (juce::AudioPluginInstance*)instrumentNode->getProcessor();
}

void Track::setProcessingPrecision(ProcessingPrecision newPrecision) {
    AudioProcessorGraph::setProcessingPrecision(newPrecision);
    for (auto it : getNodes())
        it->getProcessor()->setProcessingPrecision(newPrecision);
}

void Track::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) {
    AudioProcessorGraph::prepareToPlay(sampleRate, estimatedSamplesPerBlock);
    chain.prepare({sampleRate, (juce::uint32)estimatedSamplesPerBlock, 2});
    for (auto it : getNodes())
        it->getProcessor()->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
}

void Track::setPlayHead(juce::AudioPlayHead* newPlayHead) {
    AudioProcessorGraph::setPlayHead(newPlayHead);
    for (auto it : getNodes())
        it->getProcessor()->setPlayHead(newPlayHead);
}

void Track::setMuted(bool val) {
    if (!currentNode) return;
    auto msg = juce::MidiMessage::allNotesOff(1);
    msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    messageCollector.addMessageToQueue(msg);
    currentNode->setBypassed(val);
}

juce::DynamicObject* savePluginState(juce::MemoryBlock& memory, juce::AudioPluginInstance* instance, juce::String id, juce::File& pluginsDir) {
	instance->getStateInformation(memory);
	auto xml = instance->getXmlFromBinary(memory.getData(), (int)memory.getSize());
	if (xml == nullptr) pluginsDir.getChildFile(id + ".bin").replaceWithData(memory.getData(), memory.getSize());
	else {
		auto file = pluginsDir.getChildFile(id + ".xml");
		file.deleteFile();
		juce::FileOutputStream out(file);
		xml.release()->writeToStream(out, "");
	}
	auto obj = new juce::DynamicObject();
	obj->setProperty("id", instance->getPluginDescription().createIdentifierString());
	return obj;
}

void Track::saveState() {
	auto dir = EIMApplication::getEIMInstance()->config.projectTracksPath.getChildFile(uuid);
	auto pluginsDir = dir.getChildFile("plugins");
	dir.createDirectory();
	pluginsDir.createDirectory();
	auto obj = new juce::DynamicObject();
	juce::Array<juce::var> plguins;
	obj->setProperty("name", juce::String(name));
	obj->setProperty("color", juce::String(color));
	obj->setProperty("pan", pan);
	obj->setProperty("volume", chain.get<1>().getGainLinear());
	obj->setProperty("muted", currentNode->isBypassed());
	int i = 0;
	juce::MemoryBlock memory;
	for (auto& it : plugins) {
		plguins.add(savePluginState(memory, (juce::AudioPluginInstance*)it->getProcessor(), juce::String(i++), pluginsDir));
		memory.reset();
	}
	if (instrumentNode != nullptr) {
		obj->setProperty("instrument", savePluginState(memory, (juce::AudioPluginInstance*)instrumentNode->getProcessor(), "instrument", pluginsDir));
	}
	obj->setProperty("plugins", plguins);
	dir.getChildFile("track.json").replaceWithText(juce::JSON::toString(obj));

	if (uuid.empty()) return;
	auto midiFile = dir.getChildFile("midi.json");
	midiFile.deleteFile();
	juce::FileOutputStream midiOut(midiFile);
	midiOut << "[\n";
	int num = midiSequence.getNumEvents();
	for (auto& it : midiSequence) {
		midiOut << (int)it->message.getTimeStamp() << "," << encodeMidiMessage(it->message);
		if (--num != 0) midiOut << ',';
		midiOut << '\n';
	}
	midiOut << "]\n";
}
