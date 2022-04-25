#include "Main.h"
#include "packets.pb.h"
#include <string>

void EIMApplication::initPort(int port) {
	auto address = boost::asio::ip::make_address("0.0.0.0");

	if (listener) ioc.stop();
	listener = boost::make_shared<Listener>(ioc, boost::asio::ip::tcp::endpoint{
		address, boost::asio::ip::port_type(port) });
	listener->doAccept();

	v.clear();
	v.reserve(4);
	for (auto i = 0; i < 4; i++) { v.emplace_back([this] { ioc.run(); }).detach(); }

#ifndef JUCE_DEBUG
	juce::URL("http://127.0.0.1:" + port).launchInDefaultBrowser();
#endif
}

void EIMApplication::initialise(const juce::String& commandLine) {
    EIMPackets::ProjectStatus a;
    
    juce::ignoreUnused(commandLine);
    mainWindow.reset(new MainWindow());
    pluginManager.reset(new PluginManager(config.rootPath));
	initPort(8088);
}

void EIMApplication::shutdown() { mainWindow = nullptr; }

void EIMApplication::systemRequestedQuit() {
	ioc.stop();
    quit();
}

EIMApplication* EIMApplication::getEIMInstance() {
    return ((EIMApplication*)juce::JUCEApplication::getInstance());
}

class PortInputListener : public juce::TextEditor::Listener {
public:
	void textEditorReturnKeyPressed(juce::TextEditor&) override {}
	void textEditorEscapeKeyPressed(juce::TextEditor&) override {}
	void textEditorFocusLost(juce::TextEditor&) override {}
	void textEditorTextChanged(juce::TextEditor& editor) override {
		juce::String str;
		for (auto it : editor.getText()) {
			if (it >= '0' && it <= '9') str += it;
		}
		editor.setText(str);
	}
};

MainWindow::MainWindow(): juce::DocumentWindow("Echo In Mirror", juce::Colours::lightgrey, juce::DocumentWindow::allButtons) {
    setSize(230, 200);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    masterTrack.reset(new MasterTrack());
	masterTrack->init();

	portLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::black);
	portLabel.setText("Port:", {});
	portLabel.setBounds(10, 20, 50, 30);
	mainComponent.addAndMakeVisible(portLabel);

	portInput.setText("8088");
	portInput.addListener(new PortInputListener());
	portInput.setBounds(50, 20, 100, 30);
	mainComponent.addAndMakeVisible(portInput);

	textButton.setBounds(160, 20, 50, 30);
	textButton.setButtonText("Set!");
	textButton.onClick = [this] {
		statusLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::red);
		statusLabel.setText("The port is already occupied!", {});
		EIMApplication::getEIMInstance()->initPort(std::atoi(portInput.getText().toRawUTF8()));
	};
	mainComponent.addAndMakeVisible(textButton);

	statusLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::red);
	statusLabel.setText("The port is already occupied!", {});
	statusLabel.setBounds(10, 60, 200, 20);
	mainComponent.addAndMakeVisible(statusLabel);

	clientsLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::black);
	clientsLabel.setText("Current number of clients: 0", {});
	clientsLabel.setBounds(10, 80, 200, 20);
	mainComponent.addAndMakeVisible(clientsLabel);

	setContentNonOwned(&mainComponent, false);
}
void MainWindow::closeButtonPressed() { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

START_JUCE_APPLICATION(EIMApplication)
