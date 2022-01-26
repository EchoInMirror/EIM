#include "Config.h"

Config::Config() : rootPath(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile("EchoInMirror")),
    configPath(rootPath.getChildFile("config.json")) {
	rootPath.createDirectory();
	if (!configPath.exists()) configPath.replaceWithText("{}");
    config = juce::JSON::parse(configPath);
    auto* obj = config.getDynamicObject();
}

std::string Config::toString() { return juce::JSON::toString(config).toStdString(); }
