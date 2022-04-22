#include "PluginManager.h"
#include "../packets/packets.h"
#include "Main.h"

PluginManager::PluginManager(juce::File rootPath)
    : juce::Timer(), knownPluginListXMLFile(rootPath.getChildFile("knownPlugins.xml")),
      scannerPath(juce::File::getCurrentWorkingDirectory().getChildFile("EIMPluginScanner.exe").getFullPathName()) {
    manager.addDefaultFormats();

    if (knownPluginListXMLFile.exists()) {
        auto xml = juce::XmlDocument::parse(knownPluginListXMLFile.loadFileAsString());
        knownPluginList.recreateFromXml(*xml.get());
        xml.reset();
    }
    auto& cfg = EIMApplication::getEIMInstance()->config;
    auto obj = cfg.config.getDynamicObject();
    if (!obj->hasProperty("pluginManager")) {
        auto obj2 = new juce::DynamicObject();
        juce::var pluginManager(obj2);
        if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0) {
            obj2->setProperty("scanPaths", juce::StringArray({"C:\\Program Files\\Common Files\\VST3",
                                                              "C:\\Program Files\\VstPlugins",
                                                              "C:\\Program Files\\Steinberg\\VSTPlugins"}));
		} else if (juce::SystemStats::getOperatingSystemType() == juce::SystemStats::Linux) {
			juce::StringArray arr;
			arr.addTokens(juce::SystemStats::getEnvironmentVariable("LADSPA_PATH", "/usr/lib/ladspa;/usr/local/lib/ladspa;~/.ladspa").replace(":", ";"), ";");
			obj2->setProperty("scanPaths", arr);
		} else obj2->setProperty("scanPaths", juce::StringArray());
        obj2->setProperty("skipFiles", juce::StringArray());
        obj2->setProperty("thread", 10);
        obj->setProperty("pluginManager", pluginManager);
        cfg.changed = true;
    }
}

PluginManager::~PluginManager() { stopScanning(); }

bool PluginManager::isScanning() {
    return _isScanning;
}

void PluginManager::timerCallback() {
    if (!_isScanning) return;
    bool hasProcessRunning = false;
    for (int i = 0; i < numThreads; i++) {
        auto& cur = processes[i];
        if (cur.isRunning()) {
            hasProcessRunning = true;
            continue;
        }
        if (inited) {
			juce::StringArray tokens;

			{
				std::string tmp;
				char buf[2048] = {};
				while (true) {
					int numread = cur.readProcessOutput(buf, 2048);
					if (!numread) break;
					tmp.append(buf, numread);
				}
				tokens.addLines(tmp);
			}

			auto flag = true;
            for (auto& str : tokens) {
                auto xml = juce::XmlDocument::parse(str);
                if (xml != nullptr) {
                    juce::PluginDescription desc;
                    desc.loadFromXml(*xml.release());
                    knownPluginList.addType(desc);
					flag = false;
                }
            }
			if (flag) {
				auto& cfg = EIMApplication::getEIMInstance()->config.config.getDynamicObject()->getProperty(
					"pluginManager");
				auto arr = cfg.getProperty("skipFiles", juce::StringArray());
				auto ptr = arr.getArray();
				ptr->addIfNotAlreadyThere(processScanFile[i]);
			}
			EIMPackets::ClientboundScanningVST pkg;
			pkg.set_file(processScanFile[i].toStdString());
			pkg.set_isfinished(true);
			EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetScanningVSTPacket(pkg));
        }
        if (scainngFiles.empty()) continue;
		auto& path = scainngFiles.front();
		EIMPackets::ClientboundScanningVST pkg;
		pkg.set_count(numFiles);
		pkg.set_current(numFiles - (int)scainngFiles.size());
		pkg.set_thread(i);
		pkg.set_file(path.toStdString());
		pkg.set_isfinished(false);
		EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetScanningVSTPacket(pkg));
        cur.start(juce::StringArray{scannerPath, path});
        processScanFile[i] = path;
        scainngFiles.pop();
    }
    inited = true;
    if (!hasProcessRunning && scainngFiles.empty()) {
        stopScanning();
        return;
    }
}

void PluginManager::scanPlugins() {
    if (_isScanning) return;
	EIMPackets::ClientboundSendMessage msg;
	if (juce::File(scannerPath).existsAsFile()) {
		auto& cfg = EIMApplication::getEIMInstance()->config.config.getDynamicObject()->getProperty("pluginManager");
		numThreads = (int)cfg.getProperty("thread", 10);
		auto pathsArr = cfg.getProperty("scanPaths", juce::StringArray());
		if (pathsArr.getArray()->isEmpty()) return;
		juce::FileSearchPath paths;
		for (auto& it : *pathsArr.getArray())
			paths.add(juce::File(it));
		paths.removeRedundantPaths();
		paths.removeNonExistentPaths();
		std::queue<juce::String> empty;
		std::swap(scainngFiles, empty);
		for (auto it : manager.getFormats())
			for (auto& path : it->searchPathsForPlugins(paths, true, true)) {
				auto it = knownPluginList.getTypeForFile(path);
				if (!it || it->name.isEmpty()) scainngFiles.emplace(path);
			}
		if (scainngFiles.empty()) {
			msg.set_message("No any plugin to scan.");
		}
		else {
			_isScanning = true;
			inited = false;
			numFiles = (int)scainngFiles.size();
			processes = new juce::ChildProcess[numThreads];
			processScanFile.clear();
			processScanFile.resize(numThreads);
			startTimer(100);
			msg.set_message("Plugin scanning start!");
		}
	}
	else {
		msg.set_message("No EIMPluginScanner!");
		msg.set_type(EIMPackets::ClientboundSendMessage::MessageType::ClientboundSendMessage_MessageType_WARNING);
	}
	EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSendMessagePacket(msg));
}

void PluginManager::skipScanning(int i) {
	if (!_isScanning || i >= numThreads || !processes[i].isRunning()) return;
	processes[i].kill();
}

void PluginManager::stopScanning() {
    if (!_isScanning) return;
    DBG("Stop scan");
    auto& cfg = EIMApplication::getEIMInstance()->config;
    cfg.save();
    stopTimer();
    _isScanning = false;
	EIMPackets::ClientboundScanningVST pkg;
	pkg.set_count(0);
	pkg.set_file("");
	pkg.set_isfinished(true);
	EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetScanningVSTPacket(pkg));
    knownPluginList.createXml().release()->writeTo(knownPluginListXMLFile);
	for (int i = 0; i < numThreads; i++) {
		auto& cur = processes[i];
		if (cur.isRunning()) cur.kill();
	}
    delete[] processes;
	EIMPackets::ClientboundSendMessage msg;
	msg.set_message("Plugin scanning finished!");
	msg.set_type(EIMPackets::ClientboundSendMessage::MessageType::ClientboundSendMessage_MessageType_SUCCESS);
	EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSendMessagePacket(msg));
}

void PluginManager::createPluginWindow(juce::AudioPluginInstance* instance) {
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
