#include "Main.h"
#include <string>

void GuiAppApplication::initialise(const juce::String& commandLine) {
    juce::ignoreUnused(commandLine);
    mainWindow.reset(new MainWindow());
    auto address = boost::asio::ip::make_address("0.0.0.0");

    boost::make_shared<Listener>(ioc, boost::asio::ip::tcp::endpoint{address, 8088})->doAccept();

    v.reserve(4);
    for (auto i = 0; i < 4; i++) { v.emplace_back([this] { ioc.run(); }).detach();  }
}

void GuiAppApplication::shutdown() { mainWindow = nullptr; }

void GuiAppApplication::systemRequestedQuit() {
    ioc.stop();
    quit();
}

MainWindow::MainWindow(): juce::DocumentWindow("Echo In Mirror", juce::Colours::lightgrey, juce::DocumentWindow::allButtons) {
    setSize(400, 400);
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    masterTrack.reset(new MasterTrack());
}
void MainWindow::closeButtonPressed() { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

START_JUCE_APPLICATION(GuiAppApplication)
