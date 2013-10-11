cmake_minimum_required (VERSION 2.8.9)

project (plstim)

set (CMAKE_AUTOMOC ON)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

# Search for OpenGL (required by Windows)
find_package (OpenGL REQUIRED)
include_directories (${OPENGL_INCLUDE_DIR})

# Search for HDF5
find_package (HDF5 REQUIRED COMPONENTS CXX)
include_directories (${HDF5_INCLUDE_DIRS})

find_package (Qt5Core)

# Main program
set (plstim_src src/engine.cc src/stimwindow.cc src/utils.cc src/gui.cc src/main.cc)
qt5_wrap_cpp (plstim_moc src/qtypes.h src/setup.h)
#set (plstim_hdr src/qexperiment.h src/glwidget.h)
#qt4_wrap_cpp (plstim_moc ${plstim_hdr})
add_executable (plstim ${plstim_src} ${plstim_moc})
qt5_use_modules (plstim Core Gui Network Qml Quick)
#target_link_libraries (plstim ${QT_LIBRARIES} ${LUA_LIBRARIES} ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES})
target_link_libraries (plstim ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES})
