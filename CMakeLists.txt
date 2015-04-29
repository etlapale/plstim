cmake_minimum_required (VERSION 3.0)

project (plstim)

# Remove warning on Windows
cmake_policy (SET CMP0020 NEW)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wold-style-cast -Woverloaded-virtual -Wundef -g -pedantic")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

# Search for OpenGL (required by Windows)
find_package (OpenGL REQUIRED)
include_directories (${OPENGL_INCLUDE_DIR})

if (MINGW)
  add_definitions (-DHAVE_WIN32)
endif ()

# Search for HDF5
if (MINGW)
else ()
    find_package (HDF5 REQUIRED COMPONENTS CXX)
endif ()
include_directories (${HDF5_INCLUDE_DIRS})

# EyeLinkSupport
if (EYELINK_LIBRARIES)
  add_definitions (-DHAVE_EYELINK)
  set (eyelink_src "src/eyelink.cc")
  set (eyelink_qt_modules "Widgets")
  if (EYELINK_INCLUDE_DIRS)
    include_directories (${EYELINK_INCLUDE_DIRS})
  endif ()
endif ()

find_package (Qt5Core)

# Downloaded external dependencies
set (ext_deps
     "tests/catch.hpp https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp"
     "images/emblem-system.png http://emilien.tlapale.com/data/icons/tango-icon-theme-0.8.90/32x32/emblems/emblem-system.png")
foreach (dep IN LISTS ext_deps)
  separate_arguments (dep)
  list (GET dep 0 filename)
  list (GET dep 1 url)
  if (NOT EXISTS ${filename})
    message ("-- Downloading ${url}")
    file (DOWNLOAD ${url} ${filename})
  endif ()
endforeach ()


set (CMAKE_AUTOMOC ON)

# Library
set (libplstim_src lib/qmltypes.cc lib/setup.cc lib/utils.cc)
add_library (libplstim ${libplstim_src})
set_target_properties (libplstim PROPERTIES OUTPUT_NAME plstim)
qt5_use_modules (libplstim Core Qml Gui)

# Unit tests
file (GLOB CATCH_SRC tests/test-*.cc)
add_executable (plstim-tests EXCLUDE_FROM_ALL tests/catch-tests.cc ${CATCH_SRC})
qt5_use_modules (plstim-tests Core Qml)
target_link_libraries (plstim-tests libplstim)
add_custom_target (check)
add_dependencies (check plstim-tests)
add_custom_command (TARGET check POST_BUILD COMMAND plstim-tests)

# Main program
set (plstim_src src/engine.cc src/stimwindow.cc src/gui.cc src/qtypes.cc src/main.cc ${eyelink_src})
qt5_add_resources (plstim_qrc plstim.qrc)
add_executable (plstim ${plstim_src} ${plstim_qrc})
qt5_use_modules (plstim Core Gui Network Qml Quick ${eyelink_qt_modules})
target_link_libraries (plstim ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES} ${EYELINK_LIBRARIES} libplstim)

# vim: sw=2
