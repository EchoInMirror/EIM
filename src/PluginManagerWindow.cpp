#include "PluginManagerWindow.h"
#include "Main.h"
#include "../packets/packets.h"

class Scanner : public juce::KnownPluginList::CustomScanner {
public:
    juce::File knownPluginListXMLFile;
    juce::KnownPluginList* knownPluginList;
    bool* isScanning;
    Scanner(bool* isScanning, juce::File knownPluginListXMLFile, juce::KnownPluginList* knownPluginList) : isScanning(isScanning),
        knownPluginListXMLFile(knownPluginListXMLFile), knownPluginList(knownPluginList) { }
    bool findPluginTypesFor(juce::AudioPluginFormat& format, juce::OwnedArray<juce::PluginDescription>& result, const juce::String& fileOrIdentifier) override {
        if (!*isScanning) {
            *isScanning = true;
            EIMPackets::Boolean val;
            val.set_value(true);
            EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetIsScanningVSTsPacket(val));
        }
        format.findAllTypesForFile(result, fileOrIdentifier);
        knownPluginList->createXml().release()->writeTo(knownPluginListXMLFile);
        return true;
    }

    void scanFinished() override {
        if (*isScanning) {
            EIMPackets::Boolean val;
            val.set_value(false);
            EIMApplication::getEIMInstance()->listener->boardcast(EIMMakePackets::makeSetIsScanningVSTsPacket(val));
            *isScanning = false;
        }
        knownPluginList->createXml().release()->writeTo(knownPluginListXMLFile);
    }
};

PluginManagerWindow::PluginManagerWindow(juce::File rootPath) : juce::DocumentWindow("Plugin Manager", juce::Colours::white, juce::DocumentWindow::allButtons),
    knownPluginListXMLFile(rootPath.getChildFile("knownPlugins.xml")),
    deadMansPedalFile(rootPath.getChildFile("deadMansPedalFile.xml")),
    scanningProperties(rootPath.getChildFile("scanningProperties.xml"), { }),
pluginListComponent(std::make_unique<juce::PluginListComponent>(manager, knownPluginList, rootPath.getChildFile("deadMansPedalFile.xml"), &scanningProperties, true)) {
    pluginListComponent->setNumberOfThreadsForScanning(4);
    manager.addDefaultFormats();
    knownPluginList.setCustomScanner(std::make_unique<Scanner>(&isScanning, knownPluginListXMLFile, &knownPluginList));

    if (knownPluginListXMLFile.exists()) {
        auto xml = juce::XmlDocument::parse(knownPluginListXMLFile.loadFileAsString());
        knownPluginList.recreateFromXml(*xml.get());
        xml.reset();
    }

    setResizable(true, true);
    setContentOwned(pluginListComponent.get(), true);
    centreWithSize(getWidth(), getHeight());
}

void PluginManagerWindow::closeButtonPressed() { setVisible(false); }

void PluginManagerWindow::scanPlugins() {
    if (isScanning) return;
    setVisible(true);
    for (auto it : manager.getFormats()) {
        auto paths = juce::PluginListComponent::getLastSearchPath(scanningProperties, *it);
        paths.removeRedundantPaths();
        scanningProperties.saveIfNeeded();
        pluginListComponent->scanFor(*it, it->searchPathsForPlugins(paths, true, true));
    }
}
