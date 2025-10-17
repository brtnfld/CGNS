#!/bin/bash

# Build script for bgfx performance benchmark
# Compiles test_bgfx_performance.c with bgfx backend

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BGFX_DIR="$PROJECT_ROOT/external/bgfx"

echo "========================================="
echo "Building bgfx Performance Benchmark"
echo "========================================="
echo "Script dir: $SCRIPT_DIR"
echo "Project root: $PROJECT_ROOT"
echo "bgfx dir: $BGFX_DIR"
echo ""

# Check if bgfx exists
if [ ! -d "$BGFX_DIR" ]; then
    echo "ERROR: bgfx not found at $BGFX_DIR"
    exit 1
fi

# Build bgfx libraries if needed
cd "$BGFX_DIR"
if [ ! -f ".build/linux64_gcc/bin/libbgfxRelease.a" ]; then
    echo "Building bgfx libraries..."
    make linux-gcc-release64 -j$(nproc)
else
    echo "bgfx libraries already built"
fi

# Build shaderc if needed
if [ ! -f ".build/linux64_gcc/bin/shadercRelease" ]; then
    echo ""
    echo "Building shaderc..."
    make shaderc -j$(nproc)
else
    echo "shaderc already built"
fi

cd "$SCRIPT_DIR"

# Compile performance benchmark
echo ""
echo "Compiling performance benchmark..."
echo "------------------------------"

gcc -o test_bgfx_performance \
    -std=c99 \
    -g -O2 \
    -Wall -Wextra \
    -I"$SCRIPT_DIR" \
    -I"$BGFX_DIR/include" \
    -I"$BGFX_DIR/../bx/include" \
    -I"$BGFX_DIR/../bx/include/compat/linux" \
    -I"$BGFX_DIR/../bimg/include" \
    -DCGNS_ENABLE_BGFX \
    -DBX_CONFIG_DEBUG=0 \
    "$SCRIPT_DIR/test_bgfx_performance.c" \
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
if [ -f "./test_bgfx_performance" ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Running benchmark..."
    echo "========================================="
    ./test_bgfx_performance
    echo "========================================="
    echo ""
    echo "✓ Benchmark completed!"
else
    echo "✗ Build failed"
    exit 1
fi
