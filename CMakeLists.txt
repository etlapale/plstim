project (plstim)
cmake_minimum_required (VERSION 2.8)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

# Search for OpenGL (required by Windows)
find_package (OpenGL REQUIRED)
include_directories (${OPENGL_INCLUDE_DIR})

# Search for Qt modules
find_package (Qt4 REQUIRED)
set ( QT_USE_QTOPENGL TRUE )
set ( QT_USE_QTNETWORK TRUE )
include ( ${QT_USE_FILE} )

# Search for Lua
find_package (Lua51 REQUIRED)
include_directories (${LUA_INCLUDE_DIR})

# Search for HDF5
find_package (HDF5 REQUIRED COMPONENTS CXX)
include_directories (${HDF5_INCLUDE_DIRS})
message ("HDF5 found at ${HDF5_FOUND}, libs at ${HDF5_LIBRARIES}")

# Main program
set (plstim_src src/qexperiment.cc src/messagebox.cc src/utils.cc src/glwidget.cc)
set (plstim_hdr src/qexperiment.h src/glwidget.h)
qt4_wrap_cpp (plstim_moc ${plstim_hdr})
add_executable (plstim ${plstim_src} ${plstim_moc})
target_link_libraries (plstim ${QT_LIBRARIES} ${LUA_LIBRARIES} ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES})
