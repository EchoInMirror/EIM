#include "PluginManagerWindow.h"

class Scanner : public juce::KnownPluginList::CustomScanner {
public:
    juce::File knownPluginListXMLFile;
    juce::KnownPluginList* knownPluginList;
    Scanner(juce::File knownPluginListXMLFile, juce::KnownPluginList* knownPluginList) : knownPluginListXMLFile(knownPluginListXMLFile), knownPluginList(knownPluginList) { }
    bool findPluginTypesFor(juce::AudioPluginFormat& format, juce::OwnedArray<juce::PluginDescription>& result, const juce::String& fileOrIdentifier) override {
        format.findAllTypesForFile(result, fileOrIdentifier);
        scanFinished();
        return true;
    }

    void scanFinished() override {
        knownPluginList->createXml().release()->writeTo(knownPluginListXMLFile);
    }
};

PluginManagerWindow::PluginManagerWindow(juce::File rootPath) : juce::DocumentWindow("Plugin Manager", juce::Colours::white, juce::DocumentWindow::allButtons),
    knownPluginListXMLFile(rootPath.getChildFile("knownPlugins.xml")),
    deadMansPedalFile(rootPath.getChildFile("deadMansPedalFile.xml")),
pluginListComponent(std::make_unique<juce::PluginListComponent>(manager, knownPluginList, rootPath.getChildFile("deadMansPedalFile.xml"),
    new juce::PropertiesFile(rootPath.getChildFile("scanningProperties.xml"), { }), true)) {
    pluginListComponent->setNumberOfThreadsForScanning(4);
    manager.addDefaultFormats();
    knownPluginList.setCustomScanner(std::make_unique<Scanner>(knownPluginListXMLFile, &knownPluginList));

    if (knownPluginListXMLFile.exists()) {
        auto xml = juce::XmlDocument::parse(knownPluginListXMLFile.loadFileAsString());
        knownPluginList.recreateFromXml(*xml.get());
        xml.reset();
    }

    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setContentOwned(pluginListComponent.get(), true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void PluginManagerWindow::closeButtonPressed() { setVisible(false); }