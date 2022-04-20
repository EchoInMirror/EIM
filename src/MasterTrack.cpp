#include "MasterTrack.h"
#include "../packets/packets.h"
#include "utils/Utils.h"
#include "Main.h"

MasterTrack::MasterTrack()
    : AudioProcessorGraph(), juce::AudioPlayHead(), thumbnailCache(5), thumbnail(512, formatManager, thumbnailCache) {
    setPlayHead(this);
    thumbnail.addChangeListener(this);
    formatManager.registerBasicFormats();

    endTime = currentPositionInfo.timeSigNumerator * ppq;

    deviceManager.initialiseWithDefaultDevices(0, 2);
    setup = deviceManager.getAudioDeviceSetup();

    deviceManager.addAudioCallback(&graphPlayer);
    graphPlayer.setProcessor(this);

    setPlayConfigDetails(0, 2, setup.sampleRate == 0 ? 48000 : setup.sampleRate,
                         setup.bufferSize == 0 ? 1024 : setup.bufferSize);
    prepareToPlay(getSampleRate(), getBlockSize());
    /*
    chooser = std::make_unique<juce::FileChooser>("Select a Wave file to play...", juce::File{}, "*");
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file == juce::File{}) return;
        auto* reader = formatManager.createReaderFor(file);
        if (reader != nullptr) thumbnail.setReader(reader, 0);
    });*/
}

void MasterTrack::init() {
	auto instance = EIMApplication::getEIMInstance();
	auto& projectInfoPath = instance->config.projectInfoPath;

	auto output = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
		juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))
		->nodeID;
	if (!projectInfoPath.existsAsFile()) {
		outputNodeID = initTrack(std::make_unique<Track>(""))->nodeID;
		addConnection({ {outputNodeID, 0}, {output, 0} });
		addConnection({ {outputNodeID, 1}, {output, 1} });
		return;
	}
	auto json = juce::JSON::parse(projectInfoPath.loadFileAsString());

	ppq = (short)(int)json.getProperty("ppq", 96);
	currentPositionInfo.bpm = (int)json.getProperty("bpm", 4);
	currentPositionInfo.timeSigNumerator = (int)json.getProperty("timeSigNumerator", 4);
	currentPositionInfo.timeSigDenominator = (int)json.getProperty("timeSigDenominator", 4);
	endTime = (int)json.getProperty("endTime", currentPositionInfo.timeSigNumerator * ppq);
	auto& tracksDir = instance->config.projectTracksPath;
	auto masterTrackInfo = tracksDir.getChildFile("track.json");
	auto masterTrack = masterTrackInfo.existsAsFile() ? std::make_unique<Track>(tracksDir) : std::make_unique<Track>("");
	masterTrack->uuid = "";
	masterTrack->color = "";
	masterTrack->name = "";
	outputNodeID = initTrack(std::move(masterTrack))->nodeID;
	addConnection({ {outputNodeID, 0}, {output, 0} });
	addConnection({ {outputNodeID, 1}, {output, 1} });

	auto arr = json.getProperty("tracks", juce::StringArray());
	for (juce::String it : *arr.getArray()) {
		auto trackDir = tracksDir.getChildFile(it);
		if (trackDir.getChildFile("track.json").existsAsFile()) initTrack(std::make_unique<Track>(trackDir));
	}
}

void MasterTrack::loadPlugin(std::unique_ptr<juce::PluginDescription> desc,
                             juce::AudioPluginFormat::PluginCreationCallback callback) {
    EIMApplication::getEIMInstance()->pluginManager->manager.createPluginInstanceAsync(*desc, getSampleRate(),
                                                                                       getBlockSize(), callback);
}

void MasterTrack::loadPluginFromFile(juce::var& json, juce::File root,
	juce::AudioPluginFormat::PluginCreationCallback callback) {
	auto instance = EIMApplication::getEIMInstance();
	auto desc = instance->pluginManager->knownPluginList.getTypeForIdentifierString(json.getProperty("identifier", ""));
	if (desc == nullptr) {
		callback(nullptr, "No such plugin");
		return;
	}
	auto stateFile = json.getProperty("stateFile", "").toString();
	instance->pluginManager->manager.createPluginInstanceAsync(*desc, getSampleRate(), getBlockSize(),
		[callback, root, stateFile](std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& error) {
			if (plugin == nullptr) {
				callback(std::move(plugin), error);
				return;
			}
			auto stateFile0 = root.getChildFile(stateFile);
			if (stateFile.isNotEmpty() && stateFile0.existsAsFile()) {
				juce::MemoryBlock block;
				auto flag = true;
				if (stateFile.endsWith(".xml")) juce::AudioProcessor::copyXmlToBinary(*juce::XmlDocument::parse(stateFile0.loadFileAsString()).release(), block);
				else flag = stateFile0.loadFileAsData(block);
				if (flag) plugin->setStateInformation(block.getData(), (int)block.getSize());
			}
			callback(std::move(plugin), error);
		});
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::initTrack(std::unique_ptr<Track> track) {
    track->setRateAndBufferSizeDetails(getSampleRate(), getBlockSize());
    track->prepareToPlay(getSampleRate(), getBlockSize());
    auto& obj = *track;
    auto& node = tracks.emplace_back(addNode(std::move(track)));
    tracksMap[obj.uuid] = node;
    obj.currentNode = node;
    addConnection({{node->nodeID, 0}, {outputNodeID, 0}});
    addConnection({{node->nodeID, 1}, {outputNodeID, 1}});
    return node;
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::createTrack(std::string name, std::string color, std::string uuid) {
    juce::MidiFile file;
    auto track = std::make_unique<Track>(name, color, uuid);
    auto env = juce::SystemStats::getEnvironmentVariable("MIDI_IMPORT_PATH", "");
    if (env.isNotEmpty()) {
        juce::FileInputStream theStream(env);
        file.readFrom(theStream);
        track->addMidiEvents(*file.getTrack(1), file.getTimeFormat());
        auto newEndTime = (int)std::ceil(juce::jmax(file.getTrack(1)->getEndTime() / file.getTimeFormat(),
                                                    (double)currentPositionInfo.timeSigNumerator)) * ppq;
        if (endTime != newEndTime) {
            endTime = newEndTime;
            EIMPackets::ProjectStatus info;
            info.set_maxnotetime(endTime);
            EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
        }
    }
    return initTrack(std::move(track));
}

void MasterTrack::removeTrack(std::string uuid) {
    if (!tracksMap.contains(uuid)) return;
    auto& it = tracksMap[uuid];
    removeNode(it);
    for (auto iter = tracks.begin(); iter != tracks.end(); iter++)
        if (*iter == it) {
            tracks.erase(iter);
            break;
        }
    tracksMap.erase(uuid);
	deletedTracks.push_back(uuid);
}

bool MasterTrack::getCurrentPosition(CurrentPositionInfo& result) {
    result = currentPositionInfo;
    return true;
}

void MasterTrack::transportPlay(bool shouldStartPlaying) {
    if (currentPositionInfo.isPlaying == shouldStartPlaying) return;
    currentPositionInfo.isPlaying = shouldStartPlaying;
    EIMPackets::ProjectStatus info;
    info.set_isplaying(shouldStartPlaying);
    info.set_position((int)currentPositionInfo.ppqPosition);
    EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
}

void MasterTrack::calcPositionInfo() {
    if (!currentPositionInfo.isPlaying) return;
    currentPositionInfo.timeInSeconds = ((double)currentPositionInfo.timeInSamples) / getSampleRate();
    currentPositionInfo.ppqPosition = currentPositionInfo.timeInSeconds / 60.0 * currentPositionInfo.bpm * ppq;
    if (endTime >= currentPositionInfo.ppqPosition) return;
    currentPositionInfo.timeInSeconds = 0;
    currentPositionInfo.timeInSamples = 0;
    currentPositionInfo.ppqPosition = 0;
    EIMPackets::ProjectStatus info;
    info.set_position((int)currentPositionInfo.ppqPosition);
    EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
}

void MasterTrack::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    calcPositionInfo();
    AudioProcessorGraph::processBlock(buffer, midiMessages);
    if (currentPositionInfo.isPlaying) currentPositionInfo.timeInSamples += buffer.getNumSamples();
}

void MasterTrack::createPluginWindow(juce::AudioPluginInstance* instance) {
    if (!instance) return;
    if (pluginWindows.contains(instance)) {
        auto& it = pluginWindows.at(instance);
        it.setAlwaysOnTop(true);
        it.grabKeyboardFocus();
        it.setAlwaysOnTop(false);
    }
    else
        pluginWindows.try_emplace(instance, instance, &pluginWindows);
}

void MasterTrack::writeProjectStatus(EIMPackets::ProjectStatus& it) {
    it.set_bpm((int)currentPositionInfo.bpm);
    it.set_position((int)currentPositionInfo.ppqPosition);
    it.set_timesignumerator(currentPositionInfo.timeSigNumerator);
    it.set_timesigdenominator(currentPositionInfo.timeSigDenominator);
    it.set_ppq(ppq);
    it.set_isplaying(currentPositionInfo.isPlaying);
    it.set_maxnotetime(endTime);
}

void MasterTrack::stopAllNotes() {
    auto msg = juce::MidiMessage::allNotesOff(1);
    msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    for (auto& it : tracks)
        ((Track*)it->getProcessor())->messageCollector.addMessageToQueue(msg);
}

void MasterTrack::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &thumbnail && thumbnail.isFullyLoaded()) {
        auto length = thumbnail.getTotalLength();
        int width = juce::roundToInt(length / 60.0 * currentPositionInfo.bpm * 96 * 6);
        juce::Rectangle<int> thumbnailBounds(0, 0, width, 70);
        auto img = juce::Image(juce::Image::ARGB, width, 70, true);
        juce::Graphics g(img);
        g.setColour(juce::Colours::white);
        thumbnail.drawChannels(g, thumbnailBounds, 0.0, length, 1.0f);
        juce::PNGImageFormat format;
        auto file = juce::File::getCurrentWorkingDirectory().getChildFile("test.png");
        file.deleteFile();
        juce::FileOutputStream stream(file);
        format.writeImageToStream(img, stream);
        DBG("FINISHED");
    }
}

void MasterTrack::saveState() {
	auto obj = new juce::DynamicObject();
	obj->setProperty("ppq", ppq);
	obj->setProperty("endTime", endTime);
	obj->setProperty("bpm", currentPositionInfo.bpm);
	obj->setProperty("timeSigDenominator", currentPositionInfo.timeSigDenominator);
	obj->setProperty("timeSigNumerator", currentPositionInfo.timeSigNumerator);
	juce::StringArray trackUUIDs;
	auto& cfg = EIMApplication::getEIMInstance()->config;
	for (auto& it : tracks) {
		auto track = (Track*)it->getProcessor();
		track->saveState();
		if (!track->uuid.empty()) trackUUIDs.add(track->uuid);
	}
	for (auto& it : deletedTracks) if (!tracksMap.contains(it)) {
		auto trackDir = cfg.projectTracksPath.getChildFile(it);
		if (trackDir.exists()) trackDir.deleteRecursively();
	}
	obj->setProperty("tracks", trackUUIDs);
	cfg.projectInfoPath.replaceWithText(juce::JSON::toString(obj));
	deletedTracks.clear();
}

void MasterTrack::loadProject(juce::File newRoot) {
	auto instance = EIMApplication::getEIMInstance();
	if (newRoot == instance->config.projectRoot) return;

	clear();
	tracks.clear();
	tracksMap.clear();
	instance->undoManager.clearUndoHistory();
	instance->config.setProjectRoot(newRoot);
	init();
}
