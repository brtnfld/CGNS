##########
# common #
##########

# Render Backend Library (Phase 1: Graphics API Abstraction)
# Provides unified interface for OpenGL and bgfx backends
include(CMakeLists.txt.render_backend)

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
