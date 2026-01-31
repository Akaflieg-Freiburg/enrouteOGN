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

# Enable testing (no Qt required)
enable_testing()

# Add tests subdirectory (Qt-free tests)
add_subdirectory(tests)

# Add dumpOGN subdirectory (now also Qt-free)
add_subdirectory(dumpOGN)
