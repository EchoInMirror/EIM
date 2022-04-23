#include "Track.h"
#include "Main.h"
#include "MasterTrack.h"

Track::Track(std::string id) : uuid(id), AudioProcessorGraph() {
    init();
}

Track::Track(std::string name, std::string color, std::string uuid)
    : AudioProcessorGraph(), uuid(uuid.empty() ? randomUuid() : uuid), name(name), color(color) {
    init();
}

Track::Track(juce::File dir, std::function<void(Track*)> callback) : AudioProcessorGraph() {
	auto& masterTrack = EIMApplication::getEIMInstance()->mainWindow->masterTrack;
	auto info = juce::JSON::parse(dir.getChildFile("track.json").loadFileAsString());
	auto pluginsDir = dir.getChildFile("plugins");
	auto midiFile = dir.getChildFile("midi.json");

	name = info.getProperty("name", "Unknown Track").toString().toStdString();
	color = info.getProperty("color", "#f44336").toString().toStdString();
	uuid = info.getProperty("uuid", juce::String(randomUuid())).toString().toStdString();
	chain.get<1>().setGainLinear((float)info.getProperty("volume", 1));
	chain.get<0>().setPan((pan = (int)info.getProperty("pan", 0)) / 100.0f);
	if ((bool)info.getProperty("muted", 1)) currentNode->setBypassed(true);

	if (midiFile.existsAsFile()) {
		auto midi = juce::JSON::parse(midiFile.loadFileAsString());
		auto& arr = *midi.getArray();
		for (int i = 0, size = arr.size(); i < size; i += 2)
			midiSequence.addEvent(decodeMidiMessage(arr[i + 1], arr[i]));
	}

	if (info.hasProperty("instrument")) allPluginsCount++;

	auto pluginsArr = info.getProperty("plugins", juce::Array<juce::var>());
	for (auto& it : *pluginsArr.getArray()) {
		if (it.isObject()) allPluginsCount++;
	}

	if (info.hasProperty("instrument")) {
		auto it = info.getProperty("instrument", new juce::DynamicObject());
		masterTrack->loadPluginFromFile(it, pluginsDir,
			[this, callback](std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String&) {
				if (plugin != nullptr) setInstrument(std::move(plugin));
				if (--allPluginsCount == 0 && callback != nullptr) callback(this);
			});
	}

	for (auto& it : *pluginsArr.getArray()) {
		if (!it.isObject()) continue;
		masterTrack->loadPluginFromFile(it, pluginsDir,
			[this, callback](std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String&) {
				if (plugin != nullptr) addEffectPlugin(std::move(plugin));
				if (--allPluginsCount == 0 && callback != nullptr) callback(this);
			});
	}

	init();
}

Track::Track(Track&& other) : AudioProcessorGraph(), uuid(randomUuid()), name(other.name + " copy"), color(other.color) {
	/*name = info.getProperty("name", "Unknown Track").toString().toStdString();
	color = info.getProperty("color", "#f44336").toString().toStdString();
	uuid = info.getProperty("uuid", juce::String(randomUuid())).toString().toStdString();
	chain.get<1>().setGainLinear((float)info.getProperty("volume", 1));
	chain.get<0>().setPan((pan = (int)info.getProperty("pan", 0)) / 100.0f);*/
}

Track::~Track() {
	auto& pluginWindows = EIMApplication::getEIMInstance()->pluginManager->pluginWindows;
	if (instrumentNode) pluginWindows.erase((juce::AudioPluginInstance*)instrumentNode->getProcessor());
	for (auto& it : plugins) pluginWindows.erase((juce::AudioPluginInstance*)it->getProcessor());
	for (auto it : samples) delete it;
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
	auto found = false;
	for (auto it = plugins.begin(); it != plugins.end(); it++) if ((*it)->getProcessor() == instance) {
		plugins.erase(it);
		found = true;
		break;
	}
	if (!found) return;
	EIMApplication::getEIMInstance()->pluginManager->pluginWindows.erase(instance);
    for (auto node : getNodes()) {
		if (node->getProcessor() == instance) {
			juce::AudioProcessorGraph::NodeID prev, post;
			for (auto& conn : getConnections()) {
				if (conn.destination.nodeID == node->nodeID) prev = conn.source.nodeID;
				else post = conn.destination.nodeID;
			}
			addAudioConnection(prev, post);
			removeNode(node);
			break;
		}
    }
}

void Track::setInstrument(std::unique_ptr<juce::AudioPluginInstance> instance) {
	if (instrumentNode) {
		EIMApplication::getEIMInstance()->pluginManager->pluginWindows
			.erase((juce::AudioPluginInstance*)instrumentNode->getProcessor());
		removeNode(instrumentNode);
	}
    if (!instance) {
		instrumentNode = nullptr;
        return;
    }
    instrumentNode = addNode(std::move(instance));
    addAudioConnection(instrumentNode->nodeID, begin);
    addConnection({{midiIn, juce::AudioProcessorGraph::midiChannelIndex},
                   {instrumentNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
}

void Track::addAudioConnection(juce::AudioProcessorGraph::NodeID src, juce::AudioProcessorGraph::NodeID dest) {
    addConnection({{src, 0}, {dest, 0}});
    addConnection({{src, 1}, {dest, 1}});
}

void Track::addMidiEventsToBuffer(int sampleCount, juce::MidiBuffer& midiMessages) {
	auto& mainWindow = EIMApplication::getEIMInstance()->mainWindow;
	if (!mainWindow) return;
	auto& masterTrack = mainWindow->masterTrack;
	if (!masterTrack) return;
    auto& info = masterTrack->currentPositionInfo;
    if (info.isPlaying) {
        auto startTime = info.ppqPosition;
        auto totalTime = sampleCount / getSampleRate() / 60.0 * info.bpm * masterTrack->ppq;
        auto endTime = startTime + totalTime;
        double curTime;
        for (auto it = midiSequence.begin() + midiSequence.getNextIndexAtTime(startTime);
             it < midiSequence.end() && (curTime = (*it)->message.getTimeStamp()) < endTime; it++) {
            midiMessages.addEvent((*it)->message, juce::roundToInt((curTime - startTime) / totalTime * sampleCount));
			masterTrack->events++;
        }
    }
    messageCollector.removeNextBlockOfMessages(midiMessages, sampleCount);
}

void Track::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
	auto numSamples = buffer.getNumSamples();
    addMidiEventsToBuffer(numSamples, midiMessages);

	auto& mainWindow = EIMApplication::getEIMInstance()->mainWindow;
	if (mainWindow) {
		auto& masterTrack = mainWindow->masterTrack;
		if (masterTrack) {
			auto& info = masterTrack->currentPositionInfo;
			if (info.isPlaying) {
				auto startTime = info.ppqPosition;
				auto tmp = info.bpm * masterTrack->ppq / 60.0;
				for (auto it : samples) if (it->startPPQ <= startTime) {
					it->positionableSource.setNextReadPosition((int)(it->positionableSource.getTotalLength()
						* (startTime - it->startPPQ) / (it->fullTime == 0 ? it->info->fullTime * tmp : it->fullTime)));
					juce::AudioSourceChannelInfo channelInfo(buffer);
					it->resamplingAudioSource.getNextAudioBlock(channelInfo);
				}
			}
		}
	}

    AudioProcessorGraph::processBlock(buffer, midiMessages);
    auto inoutBlock = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, (size_t)2);
    chain.process(juce::dsp::ProcessContextReplacing<float>(inoutBlock));
	levelL = juce::jmax(buffer.getMagnitude(0, 0, numSamples), levelL);
	levelR = juce::jmax(buffer.getMagnitude(1, 0, numSamples), levelR);
}

void Track::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) {
    addMidiEventsToBuffer(buffer.getNumSamples(), midiMessages);
    AudioProcessorGraph::processBlock(buffer, midiMessages);
}

void Track::addMidiEvents(juce::MidiMessageSequence seq, int timeFormat) {
	auto& masterTrack = EIMApplication::getEIMInstance()->mainWindow->masterTrack;
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
		auto plugin = it->getProcessor();
		auto pluginName = plugin->getName();
		for (auto& str : plugin->getAlternateDisplayNames()) if (str.length() <= name.length()) pluginName = str;
        data->add_plugins()->set_name(pluginName.toStdString());
    }
    for (auto& it : midiSequence) {
        auto note = data->add_midi();
        note->set_time((int)it->message.getTimeStamp());
        note->set_data(encodeMidiMessage(it->message));
    }
	for (auto& it : samples) {
		auto note = data->add_samples();
		note->set_position(it->startPPQ);
		note->set_duration(it->info->fullTime);
		note->set_fulltime(it->fullTime);
		note->set_file(it->info->name.toStdString());
	}
}

juce::AudioPluginInstance* Track::getInstrumentInstance() {
    return instrumentNode ? (juce::AudioPluginInstance*)instrumentNode->getProcessor() : nullptr;
}

void Track::setProcessingPrecision(ProcessingPrecision newPrecision) {
    AudioProcessorGraph::setProcessingPrecision(newPrecision);
    for (auto it : getNodes())
        it->getProcessor()->setProcessingPrecision(newPrecision);
}

void Track::prepareToPlay(double sampleRate, int estimatedSamplesPerBlock) {
    AudioProcessorGraph::prepareToPlay(sampleRate, estimatedSamplesPerBlock);
    chain.prepare({sampleRate, (juce::uint32)estimatedSamplesPerBlock, 2});
	for (auto& it : samples) {
		it->resamplingAudioSource.prepareToPlay(estimatedSamplesPerBlock, sampleRate);
		it->resamplingAudioSource.setResamplingRatio(it->info->sampleRate / sampleRate);
	}
    for (auto it : getNodes())
        it->getProcessor()->prepareToPlay(sampleRate, estimatedSamplesPerBlock);
}

void Track::setRateAndBufferSizeDetails(double newSampleRate, int newBlockSize) {
	AudioProcessorGraph::setRateAndBufferSizeDetails(newSampleRate, newBlockSize);
	for (auto& it : samples) {
		it->resamplingAudioSource.prepareToPlay(newBlockSize, newSampleRate);
		it->resamplingAudioSource.setResamplingRatio(it->info->sampleRate / newSampleRate);
	}
	for (auto it : getNodes())
		it->getProcessor()->prepareToPlay(newSampleRate, newBlockSize);
	messageCollector.reset(newSampleRate);
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

void Track::addSample(SampleManager::SampleInfo* info, int startPPQ) {
	auto sample = new Sampler(info, startPPQ);
	sample->resamplingAudioSource.prepareToPlay(getBlockSize(), getSampleRate());
	samples.emplace_back(sample);
}

void Track::saveState() { saveState(EIMApplication::getEIMInstance()->config.projectTracksPath.getChildFile(uuid)); }
void Track::saveState(juce::File dir) {
	auto pluginsDir = dir.getChildFile("plugins");
	dir.createDirectory();
	pluginsDir.createDirectory();
	auto obj = new juce::DynamicObject();
	juce::Array<juce::var> plguins;
	obj->setProperty("uuid", juce::String(uuid));
	obj->setProperty("name", juce::String(name));
	obj->setProperty("color", juce::String(color));
	obj->setProperty("pan", pan);
	obj->setProperty("volume", chain.get<1>().getGainLinear());
	obj->setProperty("muted", currentNode->isBypassed());
	int i = 0;
	for (auto& it : plugins) {
		plguins.add(savePluginState((juce::AudioPluginInstance*)it->getProcessor(), juce::String(i++), pluginsDir));
	}
	if (instrumentNode != nullptr) {
		obj->setProperty("instrument", savePluginState((juce::AudioPluginInstance*)instrumentNode->getProcessor(), "instrument", pluginsDir));
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
	midiOut << "]";
}
