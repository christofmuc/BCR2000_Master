#
#  Copyright (c) 2019 Christof Ruch. All rights reserved.
#
#  Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
#

cmake_minimum_required(VERSION 3.14)

project(BCR2000_Master VERSION 0.0.1)

# Include useful scripts for CMake    
include(FetchContent REQUIRED)

# On Windows, we need to download external dependencies
if (WIN32)
    FetchContent_Declare(
	  boost
	  URL "https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.zip"
	)
	FetchContent_MakeAvailable(boost)

	FetchContent_Declare(
		vcredist
		URL https://aka.ms/vs/16/release/vc_redist.x64.exe
		DOWNLOAD_NO_EXTRACT true
	)
	FetchContent_MakeAvailable(vcredist)
	set(VCREDIST_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/vcredist-subbuild/vcredist-populate-prefix/src")
else()
    # This else branch is Linux, then.
	# Include useful scripts for CMake
	find_package(PkgConfig REQUIRED) 

	# These calls create special `PkgConfig::<MODULE>` variables
	pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk+-3.0)	
	pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.0)
	pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# New with Juce 6 - native CMake support. Let's find out how that works!
add_subdirectory(third_party/JUCE)

# Christof's own Juce6 setup
set(JUCE_MODULES_NEEDED juce::juce_core juce::juce_data_structures juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats 
			juce::juce_events  juce::juce_audio_utils juce::juce_gui_basics juce::juce_gui_extra juce::juce_dsp juce::juce_opengl)
juce_add_console_app(TOPLEVEL_JUCE PRODUCT_NAME "fake")
target_sources(TOPLEVEL_JUCE PRIVATE main.cpp)
target_link_libraries(TOPLEVEL_JUCE PRIVATE ${JUCE_MODULES_NEEDED})
juce_generate_juce_header(TOPLEVEL_JUCE)
set(JUCE_HEADER_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/TOPLEVEL_JUCE_artefacts/JuceLibraryCode/JuceHeader.h")
add_custom_target(GENERATE_JUCE_HEADER ALL DEPENDS ${JUCE_HEADER_GENERATED})
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR}/TOPLEVEL_JUCE_artefacts/JuceLibraryCode ${JUCE_MODULES_DIR})
set(COMMON_JUCE_DEFS ${CMAKE_CURRENT_BINARY_DIR}/TOPLEVEL_JUCE_artefacts/JuceLibraryCode/Debug/Defs.txt)


add_subdirectory(juce-utils)
add_subdirectory(juce-widgets)
add_subdirectory(MidiKraft-base)
add_subdirectory(MidiKraft-BCR2000)

add_subdirectory(BCRMaster)


