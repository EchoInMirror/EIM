cmake_minimum_required(VERSION 3.22)
project(EIM VERSION 0.0.0)

set(CMAKE_CXX_STANDARD 23)
set(Boost_USE_STATIC_LIBS ON)
set(Protobuf_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DJUCE_PLUGINHOST_VST3 -DJUCE_PLUGINHOST_VST)

find_package(Protobuf CONFIG REQUIRED)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS packets/packets.proto)

find_package(Boost REQUIRED)
link_directories(${Boost_LIBRARY_DIRS})

include_directories(${Boost_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SRC_FILES "src/*.cpp" "src/*.h" "src/**/*.h" "src/**/*.cpp" "packets/packets.h" "packets/packets.cpp")

add_subdirectory(JUCE)
juce_add_gui_app(EIM PRODUCT_NAME)
target_sources(EIM PRIVATE ${SRC_FILES} ${PROTO_SRCS} ${PROTO_HDRS})

if(MSVC)
    set(vst2sdkPath ${CMAKE_CURRENT_BINARY_DIR}/vst2sdk)
    set(vst2sdkDownloadPath ${vst2sdkPath}/pluginterfaces/vst2.x)
    if(NOT EXISTS ${vst2sdkDownloadPath})
        message(STATUS "Downloading VST2SDK...")
        set(vst2sdkDownloadUrl https://raw.githubusercontent.com/juce-framework/JUCE/cf4f12a452611e46d29e4549ce69adac345706b7/modules/juce_audio_processors/format_types/VST3_SDK/pluginterfaces/vst2.x)
        file(DOWNLOAD ${vst2sdkDownloadUrl}/aeffect.h ${vst2sdkDownloadPath}/aeffect.h SHOW_PROGRESS)
        file(DOWNLOAD ${vst2sdkDownloadUrl}/aeffectx.h ${vst2sdkDownloadPath}/aeffectx.h SHOW_PROGRESS)
        file(DOWNLOAD ${vst2sdkDownloadUrl}/vstfxstore.h ${vst2sdkDownloadPath}/vstfxstore.h SHOW_PROGRESS)
        message(STATUS "Downloaded VST2SDK.")
    endif()
    include_directories(${vst2sdkPath})

    add_definitions(-D_WIN32_WINNT=0x0601)
    string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")
    set_property(TARGET EIM PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif(MSVC)

target_compile_definitions(EIM
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:EIM,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:EIM,JUCE_VERSION>")

target_link_libraries(EIM
    PRIVATE
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
