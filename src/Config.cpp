#include "Config.h"

Config::Config() : rootPath(juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile("EchoInMirror")),
    configPath(rootPath.getChildFile("config.json")),
	projects(rootPath.getChildFile("projects"))
{
	rootPath.createDirectory();
	if (!configPath.exists()) configPath.replaceWithText("{}");
    config = juce::JSON::parse(configPath);
	projects.createDirectory();
	setProjectRoot(projects.getChildFile(juce::String(juce::Time::currentTimeMillis())));
	juce::Array<juce::File> files;
	projects.findChildFiles(files, juce::File::TypesOfFileToFind::findDirectories, false);
	while (files.size() > 100) {
		files[0].deleteRecursively();
		files.remove(files.begin());
	}
}

std::string Config::toString() { return juce::JSON::toString(config).toStdString(); }
void Config::save() {
	configPath.replaceWithText(config.toString());
	changed = false;
}

void Config::setProjectRoot(juce::File root) {
	projectRoot = root;
	projectRoot.createDirectory();
	projectTracksPath = root.getChildFile("tracks");
	projectTracksPath.createDirectory();
	projectInfoPath = root.getChildFile("eim-project.json");
}
