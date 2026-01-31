# dumpOGN executable

# Create dumpOGN executable
add_executable(dumpOGN dumpOGN.cpp)

# Link libraries to dumpOGN
target_link_libraries(dumpOGN
    PRIVATE
        enrouteOGN
        Qt6::Core
        Qt6::Network
        Qt6::Positioning
)

# Set automoc for dumpOGN
set_property(TARGET dumpOGN PROPERTY AUTOMOC ON)

# Include parent source directory for library headers
target_include_directories(dumpOGN
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}
)
