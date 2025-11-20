############
# cgnsplot #
############

# Option to enable bgfx rendering backend
option(CGNS_ENABLE_BGFX "Enable bgfx rendering backend for cgnsplot" OFF)

# Define source files
if (WIN32)
  if (HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
    set(cgnsplot_FILES cgnstcl.c winmain.c ../common/hash.c ../common/winhtml.c)
  else ()
    set(cgnsplot_FILES cgnstcl.c winmain.c ../common/hash.c)
  endif ()
else ()
  set(cgnsplot_FILES
	cgnstcl.c
	plotwish.c
	../common/hash.c)
endif ()

# Add render backend source if bgfx is enabled
if (CGNS_ENABLE_BGFX)
  list(APPEND cgnsplot_FILES
    ../common/render_backend_bgfx.c
    bgfx_driver.c)
endif ()

# Define executable and dependencies
add_subdirectory(../tkogl tkogl)
add_executable(plotwish ${cgnsplot_FILES})

if (CGNS_USE_SHARED)
  set(PLOT_LK_LIBS cgns_shared)
else ()
  set(PLOT_LK_LIBS cgns_static)
endif ()

if (CGNS_ENABLE_HDF5 AND HDF5_LIBRARY)
  list(APPEND PLOT_LK_LIBS ${HDF5_LIBRARY})
  if(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
    list(APPEND PLOT_LK_LIBS ${ZLIB_LIBRARY})
  endif()
  if(HDF5_NEED_SZIP AND SZIP_LIBRARY)
    list(APPEND PLOT_LK_LIBS ${SZIP_LIBRARY})
  endif()
  if(HDF5_NEED_MPI AND MPI_LIBS)
    list(APPEND PLOT_LK_LIBS ${MPI_LIBS})
  endif()
endif ()

if (WIN32)
  if (HTML_HELP_INCLUDE_PATH AND HTML_HELP_LIBRARY)
    add_definitions(-DUSE_HTMLHELP)
    target_include_directories(plotwish PRIVATE ${HTML_HELP_INCLUDE_PATH})
    list(APPEND PLOT_LK_LIBS ${HTML_HELP_LIBRARY})
  endif ()
else ()
  target_include_directories(plotwish PRIVATE ${X11_X11_INCLUDE_PATH})
  list(APPEND PLOT_LK_LIBS ${X11_X11_LIB} ${X11_Xmu_LIB} ${X11_SM_LIB} m)
endif ()

# OpenGL is needed for both traditional and bgfx modes
find_package(OpenGL REQUIRED)

# Configure bgfx backend if enabled
if (CGNS_ENABLE_BGFX)
  add_definitions(-DCGNS_ENABLE_BGFX)

  # Add bgfx include paths
  target_include_directories(plotwish PRIVATE
    ${CMAKE_SOURCE_DIR}/external/bgfx/include
    ${CMAKE_SOURCE_DIR}/external/bgfx/include/../bindings/c
    ${CMAKE_SOURCE_DIR}/external/bx/include
    ${CMAKE_SOURCE_DIR}/external/bimg/include)

  # Check if bgfx.cmake exists (CMake-based build from source)
  if (EXISTS "${CMAKE_SOURCE_DIR}/external/bgfx.cmake")
    # Use CMake-built bgfx libraries
    # Note: bgfx.cmake is included in the render_backend CMakeLists,
    # so the targets should already be defined
    if (NOT TARGET bgfx)
      # Include bgfx.cmake if not already included
      include("${CMAKE_SOURCE_DIR}/external/bgfx.cmake")
    endif()

    list(APPEND PLOT_LK_LIBS bgfx bimg bx)
    message(STATUS "Using CMake-built bgfx libraries")
  else()
    # Fall back to searching for pre-built libraries
    # Determine platform-specific library paths
    if(APPLE)
      # macOS: Check for both ARM64 and x86_64 builds
      find_library(BGFX_LIBRARY NAMES bgfx bgfxRelease
        PATHS
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx-arm64/bin
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx64_clang/bin
        NO_DEFAULT_PATH)
      find_library(BX_LIBRARY NAMES bx bxRelease
        PATHS
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx-arm64/bin
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx64_clang/bin
        NO_DEFAULT_PATH)
      find_library(BIMG_LIBRARY NAMES bimg bimgRelease
        PATHS
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx-arm64/bin
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx64_clang/bin
        NO_DEFAULT_PATH)
      find_library(BIMG_DECODE_LIBRARY NAMES bimg_decode bimg_decodeRelease
        PATHS
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx-arm64/bin
          ${CMAKE_SOURCE_DIR}/external/bgfx/.build/osx64_clang/bin
        NO_DEFAULT_PATH)
    else()
      # Linux
      find_library(BGFX_LIBRARY NAMES bgfx bgfxRelease
        PATHS ${CMAKE_SOURCE_DIR}/external/bgfx/.build/linux64_gcc/bin
        NO_DEFAULT_PATH)
      find_library(BX_LIBRARY NAMES bx bxRelease
        PATHS ${CMAKE_SOURCE_DIR}/external/bgfx/.build/linux64_gcc/bin
        NO_DEFAULT_PATH)
      find_library(BIMG_LIBRARY NAMES bimg bimgRelease
        PATHS ${CMAKE_SOURCE_DIR}/external/bgfx/.build/linux64_gcc/bin
        NO_DEFAULT_PATH)
      find_library(BIMG_DECODE_LIBRARY NAMES bimg_decode bimg_decodeRelease
        PATHS ${CMAKE_SOURCE_DIR}/external/bgfx/.build/linux64_gcc/bin
        NO_DEFAULT_PATH)
    endif()

    if (BGFX_LIBRARY AND BX_LIBRARY AND BIMG_LIBRARY)
      list(APPEND PLOT_LK_LIBS ${BGFX_LIBRARY} ${BIMG_DECODE_LIBRARY} ${BIMG_LIBRARY} ${BX_LIBRARY})
      message(STATUS "Found pre-built bgfx library: ${BGFX_LIBRARY}")
      message(STATUS "Found pre-built bx library: ${BX_LIBRARY}")
      message(STATUS "Found pre-built bimg library: ${BIMG_LIBRARY}")
    else ()
      message(WARNING "bgfx/bx/bimg libraries not found.")
      message(WARNING "Option 1: Use CMake build - ensure bgfx.cmake exists in ${CMAKE_SOURCE_DIR}/external/")
      if(APPLE)
        message(WARNING "Option 2: Build manually - cd ${CMAKE_SOURCE_DIR}/external/bgfx && make osx-arm64-release")
      else()
        message(WARNING "Option 2: Build manually - cd ${CMAKE_SOURCE_DIR}/external/bgfx && make linux-release64")
      endif()
    endif ()
  endif()

  # bgfx is a C++ library, link to standard library
  if (APPLE)
    target_link_libraries(plotwish PRIVATE tkogl ${PLOT_LK_LIBS} OpenGL::GL)
  else()
    target_link_libraries(plotwish PRIVATE tkogl ${PLOT_LK_LIBS} OpenGL::GL stdc++ dl pthread)
  endif()
else ()
  # Standard OpenGL mode
  target_link_libraries(plotwish PRIVATE tkogl ${PLOT_LK_LIBS} OpenGL::GL)
endif ()

#target_include_directories(plotwish PRIVATE ${TCL_INCLUDE_PATH} ${TK_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})
#if (X11_FOUND)
#  target_include_directories(plotwish PRIVATE ${X11_INCLUDE_DIR})
#endif()
#target_include_directories(plotwish PRIVATE ../common)

if (WIN32)
  set_target_properties(plotwish PROPERTIES
  	LINK_FLAGS /subsystem:windows)
endif ()

install(TARGETS
  plotwish
  RUNTIME DESTINATION ${CGNS_INSTALL_TOOLS_BINDIR})
if (WIN32)
  install(PROGRAMS
	cgnsplot.bat
	TYPE BIN)
else ()
  install(PROGRAMS
	cgnsplot.sh
	TYPE BIN
	RENAME cgnsplot)
  install(PROGRAMS
	cgnsplot.desktop
	TYPE BIN)
endif ()
install(FILES
  cgnsplot.tcl
  cgnsplot.ico
  cgnsplot.png
  cgnsplot-icon.xbm
  cgnsplot-mask.xbm
  DESTINATION ${CGNS_INSTALL_TOOLS_DATADIR})
