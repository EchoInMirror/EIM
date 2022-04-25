#include "Main.h"
#include "utils/Utils.h"
#include "websocket/WebSocketSession.h"

void ServerService::handleSetProjectStatus(WebSocketSession*, std::unique_ptr<EIMPackets::ProjectStatus> data) {
    auto shouldUpdate = false;
    auto instance = EIMApplication::getEIMInstance();
    auto& master = instance->mainWindow->masterTrack;
    auto& info = master->currentPositionInfo;
    if (data->has_bpm() && data->bpm() > 10) {
        info.bpm = data->bpm();
        shouldUpdate = true;
    }
    if (data->has_position()) {
        if (info.isPlaying) master->stopAllNotes();
        info.ppqPosition = (double)data->position() / master->ppq;
        info.timeInSeconds = info.ppqPosition * 60.0 / info.bpm;
        info.timeInSamples = (juce::int64)(master->getSampleRate() * info.timeInSeconds);
        data->clear_position();
    }
    if (data->has_isplaying() && data->isplaying() != info.isPlaying) {
        info.isPlaying = data->isplaying();
        if (!data->isplaying()) master->stopAllNotes();
        shouldUpdate = true;
    }
    if (data->has_timesignumerator()) {
        info.timeSigNumerator = data->timesignumerator();
        shouldUpdate = true;
    }
    if (data->has_timesigdenominator()) {
        info.timeSigDenominator = data->timesigdenominator();
        shouldUpdate = true;
    }
    if (shouldUpdate) instance->listener->boardcast(std::move(EIMMakePackets::makeSetProjectStatusPacket(*data)));
}

using ExplorerType = EIMPackets::ServerboundExplorerData::ExplorerType;
void ServerService::handleGetExplorerData(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundExplorerData> data,
                                          std::function<void(EIMPackets::ClientboundExplorerData&)> reply) {
    auto& path = data->path();
    auto instance = EIMApplication::getEIMInstance();
    EIMPackets::ClientboundExplorerData out;
    switch (data->type()) {
    case ExplorerType::ServerboundExplorerData_ExplorerType_PLUGINS:
        if (path.empty()) {
            std::unordered_set<juce::String> map;
            for (auto& it : instance->pluginManager->knownPluginList.getTypes())
                map.emplace(it.manufacturerName);
            for (auto& it : map)
                out.add_folders(it.toStdString());
        }
        else {
            for (auto& it : instance->pluginManager->knownPluginList.getTypes()) {
                if (path == it.manufacturerName)
                    out.add_files(((it.isInstrument ? "I#" : "") + it.name +
                                   (it.pluginFormatName == "VST" ? " (VST)" : "") + "#EIM#" + it.fileOrIdentifier)
                                      .toStdString());
            }
        }
        break;
    case ExplorerType::ServerboundExplorerData_ExplorerType_SAMPLES: {
        auto file = instance->config.samplesPath.getChildFile(path);
        if (file.isDirectory()) {
            for (auto& it : file.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, false)) {
                if (it.isDirectory()) out.add_folders(it.getFileName().toStdString());
                else
                    out.add_files(it.getFileName().toStdString());
            }
        }
        break;
    }
    case ExplorerType::ServerboundExplorerData_ExplorerType_MIDIs: {
        auto file = instance->config.midiPath.getChildFile(path);
        if (file.isDirectory()) {
            for (auto& it : file.findChildFiles(juce::File::TypesOfFileToFind::findFilesAndDirectories, false)) {
                if (it.isDirectory()) out.add_folders(it.getFileName().toStdString());
                else
                    out.add_files(it.getFileName().toStdString());
            }
        }
        break;
    }
    }
    reply(out);
}

void ServerService::handleRefresh(WebSocketSession* session) {
    EIMPackets::ProjectStatus data;
    EIMApplication::getEIMInstance()->mainWindow->masterTrack->writeProjectStatus(data);
    session->send(EIMMakePackets::makeSetProjectStatusPacket(data));
    EIMPackets::ClientboundTracksInfo info;
    info.set_isreplacing(true);
    auto instance = EIMApplication::getEIMInstance();
    for (auto& track : instance->mainWindow->masterTrack->tracks)
        ((Track*)track->getProcessor())->writeTrackInfo(info.add_tracks());
    session->send(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
}

void ServerService::handleOpenPluginWindow(WebSocketSession*,
                                           std::unique_ptr<EIMPackets::ServerboundOpenPluginWindow> data) {
    auto& tracks = EIMApplication::getEIMInstance()->mainWindow->masterTrack->tracksMap;
    auto& uuid = data->uuid();
    if (!tracks.contains(uuid)) return;
    auto track = (Track*)tracks[uuid]->getProcessor();
    auto plugin = data->has_index() ? track->plugins.size() > data->index() ?
                                      (juce::AudioPluginInstance*)track->plugins[data->index()]->getProcessor() :
                                      nullptr :
                                      track->getInstrumentInstance();
    if (plugin)
        juce::MessageManager::callAsync(
            [plugin] { EIMApplication::getEIMInstance()->pluginManager->createPluginWindow(plugin); });
}

void ServerService::handleConfig(WebSocketSession*, std::unique_ptr<EIMPackets::OptionalString> data,
                                 std::function<void(EIMPackets::OptionalString&)> reply) {
    auto& cfg = EIMApplication::getEIMInstance()->config;
    if (data->has_value()) {
        cfg.config = juce::JSON::parse(data->value());
        cfg.save();
    }
    else {
        data->set_value(juce::JSON::toString(cfg.config).toStdString());
    }
    reply(*data);
}

void ServerService::handleScanVSTs(WebSocketSession*) {
    EIMApplication::getEIMInstance()->pluginManager->scanPlugins();
}
void ServerService::handleSkipScanning(WebSocketSession*, std::unique_ptr<EIMPackets::Int32> data) {
    EIMApplication::getEIMInstance()->pluginManager->skipScanning(data->value());
}

void ServerService::handleSendMidiMessages(WebSocketSession*, std::unique_ptr<EIMPackets::MidiMessages> data) {
    auto& tracks = EIMApplication::getEIMInstance()->mainWindow->masterTrack->tracksMap;
    auto& uuid = data->uuid();
    if (!tracks.contains(uuid)) return;
    auto& ctrl = ((Track*)tracks[uuid]->getProcessor())->messageCollector;
    for (auto it : data->data())
        ctrl.addMessageToQueue(decodeMidiMessage(it, juce::Time::getMillisecondCounterHiRes() * 0.001));
}

void ServerService::handleUndo(WebSocketSession*) {
    DBG("undo: " << EIMApplication::getEIMInstance()->undoManager.getUndoDescription() << ": "
                 << (EIMApplication::getEIMInstance()->undoManager.undo() ? "true" : "false"));
}

void ServerService::handleRedo(WebSocketSession*) {
    DBG("redo: " << EIMApplication::getEIMInstance()->undoManager.getRedoDescription() << ": "
                 << (EIMApplication::getEIMInstance()->undoManager.redo() ? "true" : "false"));
}

void* saveState(void*) {
    EIMApplication::getEIMInstance()->mainWindow->masterTrack->saveState();
    return nullptr;
}

void ServerService::handleSave(WebSocketSession* session, std::function<void(EIMPackets::Empty&)> reply) {
    if (EIMApplication::getEIMInstance()->config.isTempProject()) handleSaveAs(session);
    else
        juce::MessageManager::getInstance()->callFunctionOnMessageThread(saveState, nullptr);
}

std::unique_ptr<juce::FileChooser> chooser;
void ServerService::handleOpenProject(WebSocketSession*) {
    chooser = std::make_unique<juce::FileChooser>("Select Project Root", juce::File{}, "*");
    runOnMainThread([] {
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                             [](const juce::FileChooser&) {
                                 if (chooser->getResult() == juce::File{}) return;
                                 EIMApplication::getEIMInstance()->mainWindow->masterTrack->loadProject(
                                     chooser->getResult());
                             });
    });
}
void ServerService::handleSaveAs(WebSocketSession*) {
    chooser = std::make_unique<juce::FileChooser>("Save As", juce::File{}, "*");
    runOnMainThread([] {
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                             [](const juce::FileChooser&) {
                                 if (chooser->getResult() == juce::File{}) return;
                                 auto instance = EIMApplication::getEIMInstance();
                                 instance->config.setProjectRoot(chooser->getResult());
                                 instance->mainWindow->masterTrack->saveState();
                             });
    });
}

void ServerService::handleBrowserPath(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundBrowserPath> data,
                                      std::function<void(EIMPackets::OptionalString&)> reply) {
    chooser =
        std::make_unique<juce::FileChooser>(data->title(), juce::File{}, data->has_patterns() ? data->patterns() : "*");
    int type = data->type();
    runOnMainThread([type, reply] {
        chooser->launchAsync(type, [reply](const juce::FileChooser&) {
            EIMPackets::OptionalString res;
            if (chooser->getResult() != juce::File{})
                res.set_value(chooser->getResult().getFullPathName().toStdString());
            reply(res);
        });
    });
}

void ServerService::handlePing(WebSocketSession*, std::function<void(EIMPackets::ClientboundPong&)> reply) {
    reply(EIMApplication::getEIMInstance()->mainWindow->masterTrack->systemInfo);
}

void ServerService::handleRender(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundRender> data,
                                 std::function<void(EIMPackets::Empty&)>) {
    auto file = juce::File(juce::String(data->path()));
    if (file.exists()) file.deleteFile();
    auto& track = EIMApplication::getEIMInstance()->mainWindow->masterTrack;
    track->render(file);
}
