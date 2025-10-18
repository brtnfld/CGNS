#!/bin/bash
#
# build_texture_test.sh
# Build script for CGNS bgfx texture tests (Phase 3)
#

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CGNS_ROOT="$SCRIPT_DIR/../../.."
BGFX_DIR="$CGNS_ROOT/external/bgfx"
BX_DIR="$CGNS_ROOT/external/bx"
BIMG_DIR="$CGNS_ROOT/external/bimg"

echo "========================================="
echo "Building CGNS bgfx Texture Tests"
echo "========================================="
echo ""
echo "Directories:"
echo "  CGNS Root: $CGNS_ROOT"
echo "  bgfx:      $BGFX_DIR"
echo "  bx:        $BX_DIR"
echo "  bimg:      $BIMG_DIR"
echo ""

# Check if bgfx exists
if [ ! -d "$BGFX_DIR" ]; then
    echo "ERROR: bgfx directory not found at $BGFX_DIR"
    echo "Please ensure bgfx is installed in external/bgfx"
    exit 1
fi

# Determine platform
UNAME_S=$(uname -s)
case "$UNAME_S" in
    Linux*)
        PLATFORM="linux"
        LIB_EXT="a"
        ;;
    Darwin*)
        PLATFORM="osx"
        LIB_EXT="a"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        PLATFORM="windows"
        LIB_EXT="lib"
        ;;
    *)
        echo "ERROR: Unknown platform: $UNAME_S"
        exit 1
        ;;
esac

echo "Platform: $PLATFORM"

# Find bgfx library
BGFX_BUILD_DIR="$BGFX_DIR/.build"
BGFX_LIB=""

# Search for bgfx library in common locations
for BUILD_TYPE in linux64_gcc osx64_clang win64_vs2019 win64_vs2022; do
    LIB_PATH="$BGFX_BUILD_DIR/$BUILD_TYPE/bin/libbgfxRelease.$LIB_EXT"
    if [ -f "$LIB_PATH" ]; then
        BGFX_LIB="$LIB_PATH"
        break
    fi
    LIB_PATH="$BGFX_BUILD_DIR/$BUILD_TYPE/bin/libbgfxDebug.$LIB_EXT"
    if [ -f "$LIB_PATH" ]; then
        BGFX_LIB="$LIB_PATH"
        break
    fi
done

if [ -z "$BGFX_LIB" ]; then
    echo "WARNING: bgfx library not found in .build directory"
    echo "Assuming bgfx is already linked or will be found by the linker"
    BGFX_LIB=""
fi

# Find bx library
BX_LIB=""
for BUILD_TYPE in linux64_gcc osx64_clang win64_vs2019 win64_vs2022; do
    LIB_PATH="$BX_DIR/.build/$BUILD_TYPE/bin/libbxRelease.$LIB_EXT"
    if [ -f "$LIB_PATH" ]; then
        BX_LIB="$LIB_PATH"
        break
    fi
    LIB_PATH="$BX_DIR/.build/$BUILD_TYPE/bin/libbxDebug.$LIB_EXT"
    if [ -f "$LIB_PATH" ]; then
        BX_LIB="$LIB_PATH"
        break
    fi
done

# Find bimg library
BIMG_LIB=""
for BUILD_TYPE in linux64_gcc osx64_clang win64_vs2019 win64_vs2022; do
    LIB_PATH="$BIMG_DIR/.build/$BUILD_TYPE/bin/libbimgRelease.$LIB_EXT"
    if [ -f "$LIB_PATH" ]; then
        BIMG_LIB="$LIB_PATH"
        break
    fi
    LIB_PATH="$BIMG_DIR/.build/$BUILD_TYPE/bin/libbimgDebug.$LIB_EXT"
    if [ -f "$LIB_PATH" ]; then
        BIMG_LIB="$LIB_PATH"
        break
    fi
done

echo ""
echo "Library locations:"
[ -n "$BGFX_LIB" ] && echo "  bgfx: $BGFX_LIB" || echo "  bgfx: (system)"
[ -n "$BX_LIB" ] && echo "  bx:   $BX_LIB" || echo "  bx:   (system)"
[ -n "$BIMG_LIB" ] && echo "  bimg: $BIMG_LIB" || echo "  bimg: (system)"
echo ""

# Compiler settings (following build_test.sh pattern)
CC=${CC:-gcc}

# Output executable
OUTPUT="test_bgfx_texture"

echo "Compiling texture tests..."
echo "------------------------------"

# Use the exact same pattern as build_test.sh which works
$CC -o "$OUTPUT" \
    -I"$SCRIPT_DIR" \
    -I"$BGFX_DIR/include" \
    -I"$BX_DIR/include" \
    -I"$BX_DIR/include/compat/linux" \
    -I"$BIMG_DIR/include" \
    -DCGNS_ENABLE_BGFX \
    -DBX_CONFIG_DEBUG=0 \
    "$SCRIPT_DIR/test_bgfx_texture.c" \
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

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "Build successful!"
    echo "========================================="
    echo ""
    echo "Run tests with: ./$OUTPUT"
    echo ""
    exit 0
else
    echo ""
    echo "========================================="
    echo "Build failed!"
    echo "========================================="
    echo ""
    exit 1
fi
