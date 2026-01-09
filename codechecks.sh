#!/bin/bash
# Code quality checks using Qt Creator's clang-tidy
# This script can be run from the enrouteOGN standalone repository
# or from the parent enroute project

set -e

CLANG_TIDY="/opt/Qt/Tools/QtCreator/libexec/qtcreator/clang/bin/clang-tidy"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Determine if we're in standalone mode or as part of enroute
if [ -f "$SCRIPT_DIR/../../build-linux/compile_commands.json" ]; then
    # Running from enroute project
    BUILD_DIR="$SCRIPT_DIR/../../build-linux"
    REPO_ROOT="$SCRIPT_DIR/../.."
else
    # Running standalone
    BUILD_DIR="$SCRIPT_DIR/build"
    REPO_ROOT="$SCRIPT_DIR"
fi

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory $BUILD_DIR does not exist. Run build script first."
    exit 1
fi

# Check if compile_commands.json exists
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "Error: compile_commands.json not found. Ensure CMake generates it."
    exit 1
fi

echo "=== Running clang-tidy on enrouteOGN library ==="
echo "Using: $("$CLANG_TIDY" --version | head -1)"
echo "Configuration: .clang-tidy"
echo

# Check all C++ files in enrouteOGN
ENROUTE_OGN_FILES=$(find "$SCRIPT_DIR/src" -name "*.cpp" -type f 2>/dev/null || true)

if [ -z "$ENROUTE_OGN_FILES" ]; then
    echo "No C++ files found in enrouteOGN"
else
    for file in $ENROUTE_OGN_FILES; do
        echo "Checking: $file"
        "$CLANG_TIDY" "$file" -p "$BUILD_DIR" || true
        echo
    done
fi

# Optional: Check enroute project files if available
ENROUTE_FILES=(
    "$REPO_ROOT/src/traffic/TrafficDataSource_Ogn.cpp"
    "$REPO_ROOT/src/traffic/TrafficDataSource_Ogn.h"
)

# Check if any enroute files exist
ENROUTE_FILES_FOUND=false
for file in "${ENROUTE_FILES[@]}"; do
    if [ -f "$file" ]; then
        ENROUTE_FILES_FOUND=true
        break
    fi
done

if [ "$ENROUTE_FILES_FOUND" = true ]; then
    echo "=== Running clang-tidy on enroute project files ==="
    echo

    for file in "${ENROUTE_FILES[@]}"; do
        if [ -f "$file" ]; then
            echo "Checking: $file"
            "$CLANG_TIDY" "$file" -p "$BUILD_DIR" --header-filter="^$file" || true
            echo
        fi
    done
fi

echo "=== Code checks completed ==="
