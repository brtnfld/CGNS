# BGFX CMake Integration for CGNS
# Simplified CMake build for bgfx, bx, and bimg libraries
#
# This is a minimal build configuration tailored for CGNS needs.
# For full bgfx features, see: https://github.com/bkaradzic/bgfx.cmake

cmake_minimum_required(VERSION 3.10)

# Enable C++ language support if not already enabled
if(NOT CMAKE_CXX_COMPILER)
    enable_language(CXX)
endif()

# Detect platform
if(WIN32)
    set(BGFX_PLATFORM "windows")
elseif(APPLE)
    set(BGFX_PLATFORM "osx")
elseif(UNIX)
    set(BGFX_PLATFORM "linux")
endif()

message(STATUS "bgfx: Building for platform: ${BGFX_PLATFORM}")

# ========================================================================
# BX Library (Base library)
# ========================================================================

set(BX_DIR "${CMAKE_CURRENT_LIST_DIR}/bx")
set(BX_INC_DIR "${BX_DIR}/include")

file(GLOB_RECURSE BX_SOURCES
    "${BX_DIR}/src/*.cpp"
)

# Remove platform-specific files we don't need
list(FILTER BX_SOURCES EXCLUDE REGEX ".*amalgamated\\.cpp$")

add_library(bx STATIC ${BX_SOURCES})

set_target_properties(bx PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(bx PUBLIC
    ${BX_INC_DIR}
)

# Only include MSVC compatibility headers on Windows
if(WIN32)
    target_include_directories(bx PUBLIC
        ${BX_INC_DIR}/compat/msvc
    )
endif()

target_compile_definitions(bx PUBLIC
    BX_CONFIG_DEBUG=$<CONFIG:Debug>
)

if(UNIX AND NOT APPLE)
    target_compile_definitions(bx PUBLIC __STDC_LIMIT_MACROS __STDC_CONSTANT_MACROS)
    target_link_libraries(bx PUBLIC pthread dl)
endif()

# ========================================================================
# BIMG Library (Image library)
# ========================================================================

set(BIMG_DIR "${CMAKE_CURRENT_LIST_DIR}/bimg")
set(BIMG_INC_DIR "${BIMG_DIR}/include")

file(GLOB BIMG_SOURCES
    "${BIMG_DIR}/src/image.cpp"
    "${BIMG_DIR}/src/image_gnf.cpp"
)

add_library(bimg STATIC ${BIMG_SOURCES})

set_target_properties(bimg PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(bimg PUBLIC
    ${BIMG_INC_DIR}
    ${BX_INC_DIR}
)

target_link_libraries(bimg PUBLIC bx)

# ========================================================================
# BGFX Library (Main rendering library)
# ========================================================================

set(BGFX_DIR "${CMAKE_CURRENT_LIST_DIR}/bgfx")
set(BGFX_INC_DIR "${BGFX_DIR}/include")
set(BGFX_SRC_DIR "${BGFX_DIR}/src")

# Core bgfx sources
file(GLOB BGFX_SOURCES
    "${BGFX_SRC_DIR}/*.cpp"
    "${BGFX_SRC_DIR}/*.h"
)

# Remove amalgamated build
list(FILTER BGFX_SOURCES EXCLUDE REGEX ".*amalgamated\\.cpp$")

# Add renderer sources based on platform
if(WIN32)
    file(GLOB BGFX_RENDERER_SOURCES
        "${BGFX_SRC_DIR}/renderer_d3d*.cpp"
        "${BGFX_SRC_DIR}/renderer_vk.cpp"
    )
elseif(APPLE)
    file(GLOB BGFX_RENDERER_SOURCES
        "${BGFX_SRC_DIR}/renderer_mtl.cpp"
        "${BGFX_SRC_DIR}/renderer_vk.cpp"
    )
else() # Linux
    file(GLOB BGFX_RENDERER_SOURCES
        "${BGFX_SRC_DIR}/renderer_vk.cpp"
        "${BGFX_SRC_DIR}/renderer_gl.cpp"
    )
endif()

list(APPEND BGFX_SOURCES ${BGFX_RENDERER_SOURCES})

# Create bgfx library
add_library(bgfx STATIC ${BGFX_SOURCES})

set_target_properties(bgfx PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(bgfx PUBLIC
    ${BGFX_INC_DIR}
    ${BGFX_SRC_DIR}
    ${BX_INC_DIR}
    ${BIMG_INC_DIR}
    ${BGFX_DIR}/3rdparty
    ${BGFX_DIR}/3rdparty/khronos
)

target_link_libraries(bgfx PUBLIC bx bimg)

# Platform-specific settings
if(WIN32)
    target_link_libraries(bgfx PUBLIC
        gdi32
        psapi
    )
    target_compile_definitions(bgfx PRIVATE
        BGFX_CONFIG_RENDERER_DIRECT3D11=1
        BGFX_CONFIG_RENDERER_DIRECT3D12=1
        BGFX_CONFIG_RENDERER_VULKAN=1
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    target_link_libraries(bgfx PUBLIC
        ${COCOA_LIBRARY}
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )
    target_compile_definitions(bgfx PRIVATE
        BGFX_CONFIG_RENDERER_METAL=1
        BGFX_CONFIG_RENDERER_VULKAN=1
    )
else() # Linux
    find_package(X11 REQUIRED)
    target_link_libraries(bgfx PUBLIC
        ${X11_LIBRARIES}
        GL
    )
    target_include_directories(bgfx PUBLIC ${X11_INCLUDE_DIR})
    target_compile_definitions(bgfx PRIVATE
        BGFX_CONFIG_RENDERER_VULKAN=1
        BGFX_CONFIG_RENDERER_OPENGL=1
    )
endif()

# Common definitions
target_compile_definitions(bgfx PUBLIC
    BGFX_CONFIG_MULTITHREADED=0  # Simplify for Phase 2
)

# C99 API
target_include_directories(bgfx PUBLIC
    ${BGFX_INC_DIR}/../bindings/c
)

# Set C++ standard (bgfx requires C++17)
set_target_properties(bgfx bx bimg PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# ========================================================================
# Summary
# ========================================================================

message(STATUS "bgfx: Configuration complete")
message(STATUS "  - bx:   ${BX_DIR}")
message(STATUS "  - bimg: ${BIMG_DIR}")
message(STATUS "  - bgfx: ${BGFX_DIR}")
