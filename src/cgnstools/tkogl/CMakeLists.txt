#########
# tkogl #
#########

# Find OpenGL package
find_package(OpenGL REQUIRED)

set(tkogl_FILES
	tkogl.c
	get.c
	gencyl.c
	load3ds.c
	nurbs.c
	quadric.c
	tess.c
	printstr.c
	feedback.c
	tkoglparse.c)

add_library(tkogl STATIC ${tkogl_FILES})

set_property(TARGET tkogl PROPERTY LINKER_LANGUAGE C)

set(TKOGL_LIBS ${TCL_LIBRARY} ${TK_LIBRARY})
if (X11_FOUND)
  list(APPEND TKOGL_LIBS ${X11_GL_LIB} ${X11_GLU_LIB})
endif()

target_link_libraries(tkogl PUBLIC ${TKOGL_LIBS} OpenGL::GL OpenGL::GLU)

target_include_directories(tkogl PUBLIC ${TCL_INCLUDE_PATH} ${TK_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})
target_include_directories(tkogl PUBLIC ../common)
if (X11_FOUND)
	target_include_directories(tkogl PUBLIC ${X11_INCLUDE_DIR})
endif()

# Add bgfx support if enabled
if(CGNS_ENABLE_BGFX)
	target_compile_definitions(tkogl PRIVATE CGNS_ENABLE_BGFX)
	# tkogl needs real OpenGL headers for widget management even when using bgfx
	target_compile_definitions(tkogl PRIVATE TKOGL_NEEDS_REAL_GL)

	# Add bgfx include paths
	get_filename_component(CGNS_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
	set(BGFX_EXTERNAL_DIR "${CGNS_SOURCE_ROOT}/external")
	target_include_directories(tkogl PUBLIC
		${BGFX_EXTERNAL_DIR}/bgfx/include
		${BGFX_EXTERNAL_DIR}/bx/include
		${BGFX_EXTERNAL_DIR}/bimg/include
	)
endif()
