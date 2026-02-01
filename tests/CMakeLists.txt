# Qt-free unit tests

# Add the test executable (no Qt dependencies)
add_executable(OgnParserTest
    OgnParserTest.cpp
    ../lib/OgnParser.cpp
)

# Include the source directory to find headers
target_include_directories(OgnParserTest
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

# No Qt dependencies - uses only C++ standard library
target_compile_features(OgnParserTest PRIVATE cxx_std_17)

# Register the test with CTest
add_test(NAME OgnParserTest COMMAND OgnParserTest)

