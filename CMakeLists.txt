cmake_minimum_required(VERSION 3.22)
project(EIM VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-DJUCE_PLUGINHOST_VST3 -DLIBUS_NO_SSL -DUWS_NO_ZLIB -DLIBUS_USE_LIBUV)
if(MSVC)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif(MSVC)

file(GLOB SRC_FILES "src/*.cpp" "src/*.h" "src/**/*.h" "src/**/*.cpp")

add_subdirectory(boost-cmake)

add_subdirectory(JUCE)
juce_add_gui_app(EIM PRODUCT_NAME)
target_sources(EIM PRIVATE ${SRC_FILES})

target_compile_definitions(EIM
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:EIM,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:EIM,JUCE_VERSION>")

target_link_libraries(EIM
    PRIVATE
        Boost::boost
        juce::juce_gui_extra
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
