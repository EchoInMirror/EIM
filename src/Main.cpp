#include "Main.h"
#include "packets.pb.h"
#include <string>

void EIMApplication::initialise(const juce::String& commandLine) {
    EIMPackets::ProjectStatus a;
    
    juce::ignoreUnused(commandLine);
    mainWindow.reset(new MainWindow());
    pluginManager.reset(new PluginManagerWindow(config.rootPath));
    auto address = boost::asio::ip::make_address("0.0.0.0");

    listener = boost::make_shared<Listener>(ioc, boost::asio::ip::tcp::endpoint{address, 8088});
    listener->doAccept();

    v.reserve(4);
    for (auto i = 0; i < 4; i++) { v.emplace_back([this] { ioc.run(); }).detach();  }
}

void EIMApplication::shutdown() { mainWindow = nullptr; }

void EIMApplication::systemRequestedQuit() {
    ioc.stop();
    quit();
}

EIMApplication* EIMApplication::getEIMInstance() {
    return ((EIMApplication*)juce::JUCEApplication::getInstance());
}

MainWindow::MainWindow(): juce::DocumentWindow("Echo In Mirror", juce::Colours::lightgrey, juce::DocumentWindow::allButtons) {
    setSize(400, 400);
    setResizable(true, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    masterTrack.reset(new MasterTrack());
}
void MainWindow::closeButtonPressed() { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

START_JUCE_APPLICATION(EIMApplication)
