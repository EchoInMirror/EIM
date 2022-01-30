#include "PluginWindow.h"

PluginWindow::PluginWindow(juce::AudioPluginInstance* instance, std::unordered_map<juce::AudioPluginInstance*, PluginWindow>* pluginWindows) :
	resizable(instance->getPluginDescription().pluginFormatName == "VST"),
	juce::DocumentWindow(
		instance->getName(),
		juce::Colours::white,
		resizable ? juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton : juce::DocumentWindow::allButtons
	), instance(instance), pluginWindows(pluginWindows)
{
	setContentOwned(instance->createEditorIfNeeded(), true);
	if (!resizable) setResizable(true, true);
	centreWithSize(getWidth(), getHeight());
	setAlwaysOnTop(true);
	setVisible(true);
	grabKeyboardFocus();
	setAlwaysOnTop(false);
}

int PluginWindow::getDesktopWindowStyleFlags() const {
	return DocumentWindow::getDesktopWindowStyleFlags();
}

void PluginWindow::closeButtonPressed() {
	pluginWindows->erase(instance);
}
