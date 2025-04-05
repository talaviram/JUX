cmake_minimum_required(VERSION 3.15)

project(Demo VERSION 0.1.0)

# Set the path to JUCE
set(JUCE_DIR "../../TICK/JUCE")

# Add JUCE
add_subdirectory(${JUCE_DIR} JUCE)

juce_add_gui_app(Demo
    VERSION 0.1.0
    COMPANY_NAME "YourCompany"
    PRODUCT_NAME "JUX Demo"
)

target_sources(Demo PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    ../components/ListBox.cpp
    ../components/ListBox.h
    ../components/ListBoxMenu.cpp
    ../components/ListBoxMenu.h
    ../components/SwitchButton.h
    ../components/TabBar.cpp
    ../components/TabBar.h
)

target_include_directories(Demo PRIVATE
    ../components
)

target_link_libraries(Demo PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
)
