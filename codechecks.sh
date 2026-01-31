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

# Track if any warnings were found
WARNINGS_FOUND=0

# Check all C++ files in lib/
LIB_FILES=$(find "$SCRIPT_DIR/lib" -name "*.cpp" -type f 2>/dev/null || true)

if [ -z "$LIB_FILES" ]; then
    echo "No C++ files found in lib/"
else
    echo "--- Checking lib/ ---"
    for file in $LIB_FILES; do
        echo "Checking: $file"
        OUTPUT=$("$CLANG_TIDY" "$file" -p "$BUILD_DIR" --header-filter="^((?!autogen|\.moc).)*$" 2>&1 || true)
        # Check if there are actual warnings (not just suppressed ones)
        if echo "$OUTPUT" | grep -q "^.*:[0-9]*:[0-9]*: warning:"; then
            echo "$OUTPUT"
            WARNINGS_FOUND=1
        fi
        echo
    done
fi

# Check all C++ files in dumpOGN/
DUMPOGN_FILES=$(find "$SCRIPT_DIR/dumpOGN" -name "*.cpp" -type f 2>/dev/null || true)

if [ -z "$DUMPOGN_FILES" ]; then
    echo "No C++ files found in dumpOGN/"
else
    echo "--- Checking dumpOGN/ ---"
    for file in $DUMPOGN_FILES; do
        echo "Checking: $file"
        OUTPUT=$("$CLANG_TIDY" "$file" -p "$BUILD_DIR" --header-filter="^((?!autogen|\.moc).)*$" 2>&1 || true)
        # Check if there are actual warnings (not just suppressed ones)
        if echo "$OUTPUT" | grep -q "^.*:[0-9]*:[0-9]*: warning:"; then
            echo "$OUTPUT"
            WARNINGS_FOUND=1
        fi
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

# Exit with appropriate code
if [ $WARNINGS_FOUND -eq 0 ]; then
    echo "✓ No warnings found - code quality checks passed!"
    exit 0
else
    echo "✗ Code quality checks failed - please fix the warnings above"
    exit 1
fi
