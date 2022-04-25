#include "MasterTrack.h"
#include "Main.h"
#include "packets.h"
#include "utils/Utils.h"

MasterTrack::MasterTrack() : AudioProcessorGraph(), juce::AudioPlayHead(), juce::Timer(), Renderable() {
    setPlayHead(this);

    endTime = currentPositionInfo.timeSigNumerator * ppq;

    deviceManager.initialiseWithDefaultDevices(0, 2);
    setup = deviceManager.getAudioDeviceSetup();

    deviceManager.addAudioCallback(&graphPlayer);
    graphPlayer.setProcessor(this);

    setPlayConfigDetails(0, 2, setup.sampleRate == 0 ? 48000 : setup.sampleRate,
                         setup.bufferSize == 0 ? 1024 : setup.bufferSize);
    prepareToPlay(getSampleRate(), getBlockSize());

    startTimer(100);
}

void MasterTrack::init() {
    auto instance = EIMApplication::getEIMInstance();
    auto& projectInfoPath = instance->config.projectInfoPath;

    auto output = addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                              juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode))
                      ->nodeID;
    if (!projectInfoPath.existsAsFile()) {
        outputNodeID = addTrack(std::make_unique<Track>(""))->nodeID;
        addConnection({{outputNodeID, 0}, {output, 0}});
        addConnection({{outputNodeID, 1}, {output, 1}});
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
    auto masterTrack =
        masterTrackInfo.existsAsFile() ? std::make_unique<Track>(tracksDir) : std::make_unique<Track>("");
    masterTrack->uuid = "";
    masterTrack->color = "";
    masterTrack->name = "";
    outputNodeID = addTrack(std::move(masterTrack))->nodeID;
    addConnection({{outputNodeID, 0}, {output, 0}});
    addConnection({{outputNodeID, 1}, {output, 1}});

    auto arr = json.getProperty("tracks", juce::StringArray());
    for (juce::String it : *arr.getArray()) {
        auto trackDir = tracksDir.getChildFile(it);
        if (trackDir.getChildFile("track.json").existsAsFile()) addTrack(std::make_unique<Track>(trackDir));
    }
}

void MasterTrack::loadPlugin(PluginState& state, juce::AudioPluginFormat::PluginCreationCallback callback) {
    auto instance = EIMApplication::getEIMInstance();
    auto desc = instance->pluginManager->knownPluginList.getTypeForIdentifierString(state.identifier);
    if (!desc) {
        callback(nullptr, "No such plugin");
        return;
    }
    instance->pluginManager->manager.createPluginInstanceAsync(
        *desc, getSampleRate(), getBlockSize(),
        [callback, &state](std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& error) {
            if (!plugin) {
                callback(nullptr, error);
                return;
            }
            plugin->setStateInformation(state.state.getData(), (int)state.state.getSize());
            callback(std::move(plugin), error);
        });
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
    if (!desc) {
        callback(nullptr, "No such plugin");
        return;
    }
    auto stateFile = json.getProperty("stateFile", "").toString();
    instance->pluginManager->manager.createPluginInstanceAsync(
        *desc, getSampleRate(), getBlockSize(),
        [callback, root, stateFile](std::unique_ptr<juce::AudioPluginInstance> plugin, const juce::String& error) {
            if (!plugin) {
                callback(nullptr, error);
                return;
            }
            auto stateFile0 = root.getChildFile(stateFile);
            if (stateFile.isNotEmpty() && stateFile0.existsAsFile()) {
                juce::MemoryBlock block;
                auto flag = true;
                if (stateFile.endsWith(".xml"))
                    juce::AudioProcessor::copyXmlToBinary(
                        *juce::XmlDocument::parse(stateFile0.loadFileAsString()).release(), block);
                else
                    flag = stateFile0.loadFileAsData(block);
                if (flag) plugin->setStateInformation(block.getData(), (int)block.getSize());
            }
            callback(std::move(plugin), error);
        });
}

void MasterTrack::checkEndTime() {

    int time = 0;
    auto tmp = currentPositionInfo.bpm * ppq / 60.0;
    for (auto& trackNode : tracks) {
        auto track = (Track*)trackNode->getProcessor();
        auto cur = (int)track->midiSequence.getEndTime();
        if (cur > time) time = cur;
        for (auto& sample : track->samples) {
            cur = (int)(sample->startPPQ + (sample->fullTime <= 0 ? sample->info->fullTime * tmp : sample->fullTime));
            if (cur > time) time = cur;
        }
    }
    checkEndTime(time);
}

void MasterTrack::checkEndTime(int time) {
    auto newEndTime = (int)std::ceil(juce::jmax(time, endTime, currentPositionInfo.timeSigNumerator * ppq));
    if (endTime != newEndTime) {
        endTime = newEndTime;
        EIMPackets::ProjectStatus info;
        info.set_maxnotetime(endTime);
        EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
    }
}

juce::AudioProcessorGraph::Node::Ptr MasterTrack::addTrack(std::unique_ptr<Track> track) {
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
    currentPositionInfo.ppqPosition = currentPositionInfo.timeInSeconds / 60.0 * currentPositionInfo.bpm;
    currentPositionInfo.ppqPositionOfLastBarStart =
        (int)(currentPositionInfo.ppqPosition / currentPositionInfo.timeSigNumerator) *
        currentPositionInfo.timeSigNumerator;
    if (endTime >= currentPositionInfo.ppqPosition * ppq) return;
    currentPositionInfo.timeInSeconds = 0;
    currentPositionInfo.timeInSamples = 0;
    currentPositionInfo.ppqPosition = 0;
    EIMPackets::ProjectStatus info;
    info.set_position((int)currentPositionInfo.ppqPosition * ppq);
    EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetProjectStatusPacket(info));
}

void MasterTrack::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    calcPositionInfo();
    AudioProcessorGraph::processBlock(buffer, midiMessages);
    if (currentPositionInfo.isPlaying) currentPositionInfo.timeInSamples += buffer.getNumSamples();
}

void MasterTrack::writeProjectStatus(EIMPackets::ProjectStatus& it) {
    it.set_bpm((int)currentPositionInfo.bpm);
    it.set_timesignumerator(currentPositionInfo.timeSigNumerator);
    it.set_timesigdenominator(currentPositionInfo.timeSigDenominator);
    it.set_ppq(ppq);
    it.set_isplaying(currentPositionInfo.isPlaying);
    it.set_maxnotetime(endTime);
    it.set_projectroot(EIMApplication::getEIMInstance()->config.projectRoot.getFullPathName().toStdString());
    it.set_projecttime(projectTime);
}

void MasterTrack::stopAllNotes() {
    auto msg = juce::MidiMessage::allNotesOff(1);
    msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
    for (auto& it : tracks)
        ((Track*)it->getProcessor())->messageCollector.addMessageToQueue(msg);
}

void MasterTrack::changeListenerCallback(juce::ChangeBroadcaster*) {
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
    for (auto& it : deletedTracks)
        if (!tracksMap.contains(it)) {
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

void MasterTrack::timerCallback() {
    EIMPackets::ClientboundPing data;
    data.set_position((int)(currentPositionInfo.ppqPosition * ppq));
    for (auto& it : tracks) {
        auto track = (Track*)it->getProcessor();
        data.add_levels(track->levelL);
        data.add_levels(track->levelR);
        track->levelL = track->levelR = 0.0f;
    }
    EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makePingPacket(data));
}

MasterTrack::SystemInfoTimer::SystemInfoTimer() {
    startTimer(1000);
}
void MasterTrack::SystemInfoTimer::timerCallback() {
    auto& masterTrack = EIMApplication::getEIMInstance()->mainWindow->masterTrack;
    masterTrack->projectTime++;
    masterTrack->systemInfo.set_cpu((int)(masterTrack->deviceManager.getCpuUsage() * 100.0));
    masterTrack->systemInfo.set_memory((int)getCurrentRSS());
    masterTrack->systemInfo.set_events(masterTrack->events);
    masterTrack->events = 0;
}

bool MasterTrack::isRenderEnd() {
    DBG(endTime << " , " << currentPositionInfo.ppqPosition);
    return endTime <= currentPositionInfo.ppqPosition * this->ppq;
}

float MasterTrack::getProgress() {
    return (float)(currentPositionInfo.ppqPosition * this->ppq) / endTime;
}

void MasterTrack::processBlockBuffer(juce::AudioBuffer<float>& buffer) {
    juce::MidiBuffer midiBuffer;
    this->processBlock(buffer, midiBuffer);
}

void MasterTrack::render(juce::File file, std::unique_ptr<EIMPackets::ServerboundRender> data) {
    this->outStream = std::move(file.createOutputStream());
    if (this->outStream == nullptr) {
        DBG("error in open file");
        return;
    }
    juce::AudioProcessor* pre = this->graphPlayer.getCurrentProcessor();
    this->graphPlayer.setProcessor(nullptr);
    juce::AudioPlayHead::CurrentPositionInfo copyInfo = this->currentPositionInfo;
    this->currentPositionInfo.ppqPosition = 0;
    this->currentPositionInfo.timeInSamples = 0;
    this->currentPositionInfo.timeInSeconds = 0;
    this->currentPositionInfo.editOriginTime = 0;
    juce::WavAudioFormat format;
    juce::StringPairArray pair;
    DBG("start render : " << this->getSampleRate() << "; " << this->getNumOutputChannels());
    this->bufferBlockSize = data->has_bitspresample() ? (int)data->bitspresample() : this->getBlockSize();
    double sampleRate = data->has_samplerate() ? data->samplerate() : this->getSampleRate();
    this->audioWirte.reset(
        format.createWriterFor(this->outStream.get(), sampleRate, this->getNumOutputChannels(), 16, pair, 0));
    this->currentPositionInfo.isPlaying = true;
    this->renderer = std::make_unique<Renderer>();
    this->renderer->render(this, std::move(audioWirte), [this, copyInfo, pre]() {
        DBG("call back");
        this->currentPositionInfo = copyInfo;
        this->graphPlayer.setProcessor(pre);
    });
}
