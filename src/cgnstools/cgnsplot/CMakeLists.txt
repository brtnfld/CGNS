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
  list(APPEND cgnsplot_FILES ../common/render_backend_bgfx.c)
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

# Configure bgfx backend if enabled
if (CGNS_ENABLE_BGFX)
  add_definitions(-DCGNS_ENABLE_BGFX)

  # Add bgfx include paths
  target_include_directories(plotwish PRIVATE
    ${CMAKE_SOURCE_DIR}/external/bgfx/include
    ${CMAKE_SOURCE_DIR}/external/bx/include
    ${CMAKE_SOURCE_DIR}/external/bimg/include)

  # Add bgfx, bx, and bimg libraries
  # Note: bgfx needs to be built separately (e.g., make linux-release64)
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

  if (BGFX_LIBRARY AND BX_LIBRARY AND BIMG_LIBRARY)
    list(APPEND PLOT_LK_LIBS ${BGFX_LIBRARY} ${BIMG_DECODE_LIBRARY} ${BIMG_LIBRARY} ${BX_LIBRARY})
    message(STATUS "Found bgfx library: ${BGFX_LIBRARY}")
    message(STATUS "Found bx library: ${BX_LIBRARY}")
    message(STATUS "Found bimg library: ${BIMG_LIBRARY}")
    message(STATUS "Found bimg_decode library: ${BIMG_DECODE_LIBRARY}")
  else ()
    message(WARNING "bgfx/bx/bimg libraries not found. Please build bgfx first:
      cd ${CMAKE_SOURCE_DIR}/external/bgfx
      make linux-release64")
  endif ()

  # bgfx backend still needs some GL for platform integration
  # but doesn't use OpenGL for rendering
  # bgfx is a C++ library, so we need to link against stdc++
  # bgfx also needs dl (dynamic linking) and pthread
  target_link_libraries(plotwish PRIVATE tkogl ${PLOT_LK_LIBS} OpenGL::GL stdc++ dl pthread)
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
