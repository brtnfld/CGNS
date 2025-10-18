#!/bin/bash
# Compile textured shaders for Phase 3
# Generates GLSL and SPIR-V headers for textured rendering

SHADERC="../../../../external/bgfx/tools/bin/linux/shaderc"
BGFX_INCLUDE="../../../../external/bgfx/src"
VARYING="varying_textured.def.sc"

echo "=== Compiling Textured Shaders for Phase 3 ==="
echo ""

# Check if shaderc exists
if [ ! -f "$SHADERC" ]; then
    echo "ERROR: shaderc not found at $SHADERC"
    echo "Please build bgfx first: cd external/bgfx && make shaderc"
    exit 1
fi

# Textured Vertex shader
echo "Compiling vs_textured..."
$SHADERC -f vs_textured.sc -o vs_textured_glsl.h \
    --type vertex --platform linux -p 120 \
    --bin2c vs_textured_glsl \
    -i $BGFX_INCLUDE --varyingdef $VARYING

$SHADERC -f vs_textured.sc -o vs_textured_spirv.h \
    --type vertex --platform linux -p spirv \
    --bin2c vs_textured_spirv \
    -i $BGFX_INCLUDE --varyingdef $VARYING

# Textured Fragment shaders - Smooth
echo "Compiling fs_textured_smooth..."
$SHADERC -f fs_textured_smooth.sc -o fs_textured_smooth_glsl.h \
    --type fragment --platform linux -p 120 \
    --bin2c fs_textured_smooth_glsl \
    -i $BGFX_INCLUDE --varyingdef $VARYING

$SHADERC -f fs_textured_smooth.sc -o fs_textured_smooth_spirv.h \
    --type fragment --platform linux -p spirv \
    --bin2c fs_textured_smooth_spirv \
    -i $BGFX_INCLUDE --varyingdef $VARYING

# Textured Fragment shaders - Flat
echo "Compiling fs_textured_flat..."
$SHADERC -f fs_textured_flat.sc -o fs_textured_flat_glsl.h \
    --type fragment --platform linux -p 120 \
    --bin2c fs_textured_flat_glsl \
    -i $BGFX_INCLUDE --varyingdef $VARYING

$SHADERC -f fs_textured_flat.sc -o fs_textured_flat_spirv.h \
    --type fragment --platform linux -p spirv \
    --bin2c fs_textured_flat_spirv \
    -i $BGFX_INCLUDE --varyingdef $VARYING

# Textured Fragment shaders - Unlit
echo "Compiling fs_textured_unlit..."
$SHADERC -f fs_textured_unlit.sc -o fs_textured_unlit_glsl.h \
    --type fragment --platform linux -p 120 \
    --bin2c fs_textured_unlit_glsl \
    -i $BGFX_INCLUDE --varyingdef $VARYING

$SHADERC -f fs_textured_unlit.sc -o fs_textured_unlit_spirv.h \
    --type fragment --platform linux -p spirv \
    --bin2c fs_textured_unlit_spirv \
    -i $BGFX_INCLUDE --varyingdef $VARYING

echo ""
echo "=== Compilation Complete ==="
echo "Textured shader headers generated:"
ls -lh vs_textured*.h fs_textured*.h 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "Total textured shader headers: $(ls vs_textured*.h fs_textured*.h 2>/dev/null | wc -l)"
