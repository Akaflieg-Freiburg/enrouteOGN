# Find the Qt6Test, Qt6Network, Qt6Location, and Qt6Bluetooth modules
find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat Test Network Location Positioning)

# Add the test executable
add_executable(unittestOgn
    TrafficDataSource_OgnTest.cpp
    ../src/TrafficDataSource_OgnParser.cpp
)

# Include the source directory to find headers
target_include_directories(unittestOgn
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Link the test executable with the necessary libraries
target_link_libraries(unittestOgn
    PRIVATE
    Qt6::Core
    Qt6::Core5Compat
    Qt6::Test
    Qt6::Network
    Qt6::Location
    Qt6::Positioning
)

# Add compile definitions specific to this target
target_compile_definitions(unittestOgn
    PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x051200
)

# Copy the received_data.txt file to the build directory
configure_file(received_data.txt ${CMAKE_CURRENT_BINARY_DIR}/received_data.txt COPYONLY)

# Register the unittestOgn executable with CTest
add_test(NAME unittestOgn COMMAND unittestOgn)
