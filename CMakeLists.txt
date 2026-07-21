cmake_minimum_required(VERSION 3.22)

project(Reverb402 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(Reverb402
    COMPANY_NAME "UNTREF"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    FORMATS VST3 Standalone
    PRODUCT_NAME "Reverb402"
    PLUGIN_CODE "R402"
    PLUGIN_MANUFACTURER_CODE "UNTF"
    BUNDLE_ID "com.untref.reverb402"
    VERSION "1.0.0"
)

target_sources(Reverb402 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Componentes/IRVisualizer.cpp
    Source/Componentes/MedidorNivel.cpp
    Source/Componentes/EscalaDb.cpp
    Source/Estilos/Estilo402.cpp
    Source/Estilos/EstiloPushBtn.cpp
    Source/Estilos/EstiloKnobGain.cpp
)

juce_add_binary_data(Reverb402_resources
    SOURCES
        Resources/Fonts/Orbitron/static/Orbitron-Black.ttf
        Resources/Fonts/Orbitron/static/Orbitron-Medium.ttf
        Resources/Fonts/Chivo_Mono/static/ChivoMono-SemiBold.ttf
        Resources/Textures/aluminio.jpg
)

target_link_libraries(Reverb402 PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_audio_plugin_client
    Reverb402_resources
)

juce_generate_juce_header(Reverb402)

target_compile_definitions(Reverb402 PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if (MSVC)
    add_compile_options(/utf-8)
endif()