#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "MasterTrack.h"

class MainWindow: public juce::DocumentWindow {
public:
    MainWindow();
    ~MainWindow() { masterTrack = nullptr; }

    std::unique_ptr<MasterTrack> masterTrack;
    void closeButtonPressed() override;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class GuiAppApplication : public juce::JUCEApplication {
public:
    GuiAppApplication() {}
    std::unique_ptr<MainWindow> mainWindow;

    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override;

    void shutdown() override;
    void systemRequestedQuit() override;
private:
    boost::asio::io_context ioc{ 4 };
    std::vector<std::thread> v;
};
