#pragma once

#include "MasterTrack.h"
#include "websocket/Listener.h"

class MainWindow: public juce::DocumentWindow {
public:
    MainWindow();
    ~MainWindow() { masterTrack = nullptr; }

    std::unique_ptr<MasterTrack> masterTrack;
    void closeButtonPressed() override;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class EIMApplication : public juce::JUCEApplication {
public:
    EIMApplication() {}
    std::unique_ptr<MainWindow> mainWindow;
    boost::shared_ptr<Listener> listener;

    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override;

    void shutdown() override;
    void systemRequestedQuit() override;
    void handlePacket(WebSocketSession* buf);
    static EIMApplication* getEIMInstance();
private:
    boost::asio::io_context ioc{ 4 };
    std::vector<std::thread> v;
};
