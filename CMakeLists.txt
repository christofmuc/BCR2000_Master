#
#  Copyright (c) 2019 Christof Ruch. All rights reserved.
#
#  Dual licensed: Distributed under Affero GPL license by default, an MIT license is available for purchase
#

cmake_minimum_required(VERSION 3.14)

project(BCR2000_Master)

# Include useful scripts for CMake    
include(FetchContent REQUIRED)

# On Windows, we need to download external dependencies
if (WIN32)
    FetchContent_Declare(
	  boost
	  URL "https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.zip"
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

# We need a little bit support for the JUCE library, and also want to put our own CMake helpers on the module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/third_party/juce-cmake/cmake" "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_subdirectory(juce-utils)
add_subdirectory(juce-widgets)
add_subdirectory(MidiKraft-base)
add_subdirectory(MidiKraft-BCR2000)

add_subdirectory(BCRMaster)


