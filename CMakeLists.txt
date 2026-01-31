cmake_minimum_required(VERSION 3.16)

project(enrouteOGN VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Positioning Network)

# Set automoc for Qt
set(CMAKE_AUTOMOC ON)

# Source files
set(SOURCES
    lib/TrafficDataSource_OgnParser.cpp
)

# Header files
set(HEADERS
    lib/TrafficDataSource_OgnParser.h
)

# Create static library
add_library(enrouteOGN STATIC ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(enrouteOGN
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>
)

# Link Qt libraries
target_link_libraries(enrouteOGN
    PUBLIC
        Qt6::Core
        Qt6::Positioning
)

# Set properties
set_target_properties(enrouteOGN PROPERTIES
    VERSION ${PROJECT_VERSION}
    POSITION_INDEPENDENT_CODE ON
)

# Add dumpOGN subdirectory
add_subdirectory(dumpOGN)

# Enable testing
enable_testing()

# Add tests subdirectory
add_subdirectory(tests)
