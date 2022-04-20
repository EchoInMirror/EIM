#include "PluginManager.h"
#include "../packets/packets.h"
#include "Main.h"

PluginManager::PluginManager(juce::File rootPath)
    : juce::Timer(), knownPluginListXMLFile(rootPath.getChildFile("knownPlugins.xml")),
      scannerPath(juce::File::getCurrentWorkingDirectory().getChildFile("EIMPluginScanner.exe").getFileName()) {
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
        }
        obj2->setProperty("skipFiles", juce::StringArray());
        obj2->setProperty("thread", 10);
        obj->setProperty("pluginManager", pluginManager);
        cfg.changed = true;
    }
}

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
        DBG("" << i << " " << this->processScanFile[i] << " running");
        if (inited) {
            char arr[10240] = {};
            juce::String str(arr, cur.readProcessOutput(arr, 10240));
			DBG(str);
            auto xml = juce::XmlDocument::parse(str);
            if (xml != nullptr) {
				DBG(xml->toString());
                juce::PluginDescription desc;
                desc.loadFromXml(*xml.release());
                knownPluginList.addType(desc);
                DBG("Scanned: " << str);
            }
            else {
				DBG("skip " << this->processScanFile[i]);
                auto& cfg =
                    EIMApplication::getEIMInstance()->config.config.getDynamicObject()->getProperty("pluginManager");
                cfg.getProperty("skipFiles", juce::StringArray()).getArray()->add(this->processScanFile[i]);
            }
        }
        if (scainngFiles.empty()) {
            continue;
        }
        auto& path = scainngFiles.front();
        cur.start(juce::StringArray{scannerPath, path});
        this->processScanFile[i] = path;
        scainngFiles.pop();
    }
    inited = true;
    if (!hasProcessRunning && scainngFiles.empty()) {
        stopScanning();
        return;
    }
    // TODO: Check process status and relaunch
    // and finally stopTimer();
}

void PluginManager::scanPlugins() {
    if (_isScanning) return;
    std::thread thread([this]() {
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
            for (auto& path : it->searchPathsForPlugins(paths, true, true))
                scainngFiles.emplace(path);
        _isScanning = true;
        inited = false;
        processes = new juce::ChildProcess[numThreads];
        processScanFile.clear();
        processScanFile.resize(numThreads);
        startTimer(100);
    });
    thread.detach();
}

void PluginManager::stopScanning() {
    if (!_isScanning) return;
	DBG("stop scanning");
    auto& cfg = EIMApplication::getEIMInstance()->config;
    cfg.save();
    stopTimer();
    _isScanning = false;
    knownPluginList.createXml().release()->writeTo(knownPluginListXMLFile);
    delete[] processes;
}
