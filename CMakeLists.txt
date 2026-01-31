cmake_minimum_required(VERSION 3.16)

project(enrouteOGN VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    lib/OgnParser.cpp
)

# Header files
set(HEADERS
    lib/OgnParser.h
)

# Create static library (Qt-free, uses only C++ standard library)
add_library(enrouteOGN STATIC ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(enrouteOGN
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>
)

# No Qt dependencies - library uses only C++ standard library

# Set properties
set_target_properties(enrouteOGN PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
)

# Determine if subdirectories should be excluded from default build
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Standalone build - include everything in default build
    set(ENROUTE_OGN_EXCLUDE_FROM_ALL "")
    enable_testing()
else()
    # Integrated build - exclude tests and utilities from default build
    set(ENROUTE_OGN_EXCLUDE_FROM_ALL "EXCLUDE_FROM_ALL")
endif()

# Add subdirectories (always configure, but exclude from default build when integrated)
add_subdirectory(tests ${ENROUTE_OGN_EXCLUDE_FROM_ALL})
add_subdirectory(dumpOGN ${ENROUTE_OGN_EXCLUDE_FROM_ALL})
