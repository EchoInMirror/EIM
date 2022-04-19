#include "Main.h"
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
		info.ppqPosition = data->position();
		info.timeInSeconds = info.ppqPosition * 60.0 / info.bpm / master->ppq;
		info.timeInSamples = (juce::int64)(master->getSampleRate() * info.timeInSeconds);
		shouldUpdate = true;
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
	data->set_position((int)info.ppqPosition);
	if (shouldUpdate) instance->listener->boardcast(std::move(EIMMakePackets::makeSetProjectStatusPacket(*data)));
}

void ServerService::handleGetExplorerData(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundExplorerData> data, std::function<void(EIMPackets::ClientboundExplorerData&)> reply) {
	auto& path = data->path();
	auto instance = EIMApplication::getEIMInstance();
	EIMPackets::ClientboundExplorerData out;
	switch (data->type()) {
	case EIMPackets::ServerboundExplorerData::ExplorerType::ServerboundExplorerData_ExplorerType_PLUGINS:
		if (path.empty()) {
			std::unordered_set<juce::String> map;
			for (auto& it : instance->pluginManager->knownPluginList.getTypes()) map.emplace(it.manufacturerName);
			for (auto& it : map) out.add_folders(it.toStdString());
		} else {
			std::vector<juce::String> arr;
			for (auto& it : instance->pluginManager->knownPluginList.getTypes()) {
				if (path == it.manufacturerName) arr.emplace_back((it.isInstrument ? "I#" : "") + it.name +
					(it.pluginFormatName == "VST" ? " (VST)" : "") + "#EIM#" + it.fileOrIdentifier);
			}
			for (auto& it : arr) out.add_files(it.toStdString());
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
	for (auto& track : instance->mainWindow->masterTrack->tracks) ((Track*)track->getProcessor())->writeTrackInfo(info.add_tracks());
	session->send(std::move(EIMMakePackets::makeSyncTracksInfoPacket(info)));
	if (EIMApplication::getEIMInstance()->pluginManager->isScanning()) {
		EIMPackets::Boolean val;
		val.set_value(true);
		session->send(EIMMakePackets::makeSetIsScanningVSTsPacket(val));
	}
}

void ServerService::handleOpenPluginWindow(WebSocketSession*, std::unique_ptr<EIMPackets::ServerboundOpenPluginWindow> data) {
	auto& tracks = EIMApplication::getEIMInstance()->mainWindow->masterTrack->tracksMap;
	auto& uuid = data->uuid();
	if (!tracks.contains(uuid)) return;
	auto track = (Track*) tracks[uuid]->getProcessor();
	auto plugin = data->has_index()
		? track->plugins.size() > data->index() ? (juce::AudioPluginInstance*)track->plugins[data->index()]->getProcessor() : nullptr
		: track->getInstrumentInstance();
	if (plugin) juce::MessageManager::callAsync([plugin] { EIMApplication::getEIMInstance()->mainWindow->masterTrack->createPluginWindow(plugin); });
}

void ServerService::handleConfig(WebSocketSession*, std::unique_ptr<EIMPackets::OptionalString> data, std::function<void(EIMPackets::OptionalString&)> reply) {
	auto instance = EIMApplication::getEIMInstance();
	auto& cfg = instance->config;
	auto& manager = instance->pluginManager;
	if (data->has_value()) {
		cfg.config = juce::JSON::parse(data->value());
		data->clear_value();
	} else {
		data->set_value(juce::JSON::toString(cfg.config).toStdString());
	}
	reply(*data);
}

void ServerService::handleScanVSTs(WebSocketSession*) {
	juce::MessageManager::callAsync([] { EIMApplication::getEIMInstance()->pluginManager->scanPlugins(); });
}

void ServerService::handleSendMidiMessages(WebSocketSession*, std::unique_ptr<EIMPackets::MidiMessages> data) {
	auto& tracks = EIMApplication::getEIMInstance()->mainWindow->masterTrack->tracksMap;
	auto& uuid = data->uuid();
	if (!tracks.contains(uuid)) return;
	auto& ctrl = ((Track*)tracks[uuid]->getProcessor())->messageCollector;
	for (auto it : data->data()) ctrl.addMessageToQueue(decodeMidiMessage(it, juce::Time::getMillisecondCounterHiRes() * 0.001));
}

void ServerService::handleUndo(WebSocketSession*) {
	DBG("undo");
	EIMApplication::getEIMInstance()->undoManager.undo();
}

void ServerService::handleRedo(WebSocketSession*) {
	DBG("redo");
	EIMApplication::getEIMInstance()->undoManager.redo();
}
