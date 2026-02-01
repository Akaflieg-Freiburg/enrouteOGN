#!/bin/bash

# Build and test script for enrouteOGN library
# This script builds the library and runs the unit tests

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}====================================${NC}"
echo -e "${GREEN}enrouteOGN Build and Test Script${NC}"
echo -e "${GREEN}====================================${NC}"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir -p build
cd build

# Configure with CMake (no Qt required)
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_FLAGS="-Werror"

# Build
echo -e "${YELLOW}Building library and tests...${NC}"
cmake --build . -j$(nproc)

# Run tests
echo -e "${YELLOW}Running tests...${NC}"
ctest --output-on-failure --verbose

# Check result
if [ $? -eq 0 ]; then
    echo -e "${GREEN}====================================${NC}"
    echo -e "${GREEN}All tests passed successfully!${NC}"
    echo -e "${GREEN}====================================${NC}"
    exit 0
else
    echo -e "${RED}====================================${NC}"
    echo -e "${RED}Tests failed!${NC}"
    echo -e "${RED}====================================${NC}"
    exit 1
fi
