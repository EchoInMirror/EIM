#include "Config.h"

Config::Config() : rootPath(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile("EchoInMirror")),
    configPath(rootPath.getChildFile("config.json")) {
	rootPath.createDirectory();
	if (!configPath.exists()) configPath.replaceWithText("{}");
    config = juce::JSON::parse(configPath);
}

std::string Config::toString() { return juce::JSON::toString(config).toStdString(); }
void Config::save() {
	configPath.replaceWithText(config.toString());
	changed = false;
}
