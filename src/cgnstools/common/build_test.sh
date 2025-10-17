#!/bin/bash
#
# Standalone build script for bgfx backend test
# This script builds and runs the bgfx backend test without needing full CGNS build
#

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BGFX_DIR="$PROJECT_ROOT/external/bgfx"

echo "========================================="
echo "Building bgfx Backend Test"
echo "========================================="
echo "Script dir: $SCRIPT_DIR"
echo "Project root: $PROJECT_ROOT"
echo "bgfx dir: $BGFX_DIR"
echo ""

# Check if bgfx is available
if [ ! -d "$BGFX_DIR" ]; then
    echo "ERROR: bgfx not found at $BGFX_DIR"
    echo "Please run the Phase 2 setup first"
    exit 1
fi

# Check if bgfx libraries are built
BGFX_LIB="$BGFX_DIR/.build/linux64_gcc/bin/libbgfxRelease.a"
if [ ! -f "$BGFX_LIB" ]; then
    echo "Building bgfx libraries..."
    (cd "$BGFX_DIR" && make linux-gcc-release64 -j$(nproc))
fi

# Check if shaderc is built
SHADERC="$BGFX_DIR/.build/linux64_gcc/bin/shaderc"
if [ ! -f "$SHADERC" ]; then
    echo "Building shaderc..."
    (cd "$BGFX_DIR" && make shaderc -j$(nproc))
fi

# Compile the test program directly
echo ""
echo "Compiling test_bgfx_simple..."
echo "------------------------------"

gcc -o test_bgfx_simple \
    -I"$SCRIPT_DIR" \
    -I"$BGFX_DIR/include" \
    -I"$BGFX_DIR/../bx/include" \
    -I"$BGFX_DIR/../bx/include/compat/linux" \
    -I"$BGFX_DIR/../bimg/include" \
    -DCGNS_ENABLE_BGFX \
    -DBX_CONFIG_DEBUG=0 \
    "$SCRIPT_DIR/test_bgfx_simple.c" \
    "$SCRIPT_DIR/render_backend_bgfx.c" \
    -L"$BGFX_DIR/.build/linux64_gcc/bin" \
    -lbgfxRelease \
    -lbxRelease \
    -lbimgRelease \
    -lbimg_decodeRelease \
    -lGL \
    -lGLU \
    -lX11 \
    -ldl \
    -lpthread \
    -lm \
    -lstdc++

echo ""
if [ -f "./test_bgfx_simple" ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Running test..."
    echo "========================================="
    ./test_bgfx_simple
    TEST_RESULT=$?
    echo "========================================="
    echo ""
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✓ Test PASSED!"
    else
        echo "✗ Test FAILED (exit code: $TEST_RESULT)"
    fi
    exit $TEST_RESULT
else
    echo "✗ Build failed"
    exit 1
fi
