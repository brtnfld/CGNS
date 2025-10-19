##########
# common #
##########

# Render Backend Library (Phase 1: Graphics API Abstraction)
# Provides unified interface for OpenGL and bgfx backends
# NOTE: Only build render_backend library when building standalone tests.
# cgnsplot uses pre-built bgfx library and doesn't need to build from source.
if(CGNS_ENABLE_TESTS AND CGNS_ENABLE_BGFX)
  include(CMakeLists.txt.render_backend)
endif()

install(FILES
  balloon.tcl
  color.tcl
  combobox.tcl
  config.tcl
  dialog.tcl
  editfile.tcl
  filesel.tcl
  findfile.tcl
  frame.tcl
  help.tcl
  menubar.tcl
  tclIndex
  tclreg.tcl
  tkdir.tcl
  tree.tcl
  units.tcl
  cgns.ico
  cgns.png
  cgns-icon.xbm
  cgns-mask.xbm
  DESTINATION ${CGNS_INSTALL_TOOLS_DATADIR})
