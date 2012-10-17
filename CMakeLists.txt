project (plstim)
cmake_minimum_required (VERSION 2.8)
include (FindPkgConfig)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")
include_directories (".")

# Search for Qt modules
find_package (Qt4 REQUIRED)
set ( QT_USE_QTOPENGL TRUE )
include ( ${QT_USE_FILE} )

# Search for other libraries
pkg_check_modules (YAMLCPP REQUIRED yaml-cpp)

# Shared library
set (lib_src lib/qexperiment.cc lib/setup.cc)
set (lib_hdr lib/qexperiment.h lib/glwidget.h)
qt4_wrap_cpp (lib_moc ${lib_hdr})
add_library (plstim SHARED ${lib_src} ${lib_moc})

# Experiments
set (se_src apps/simple-experiment/simple-experiment.cc)
qt4_automoc (${se_src})
add_executable (simple-experiment ${se_src})
target_link_libraries (simple-experiment
  plstim ${QT_LIBRARIES} ${YAMLCPP_LIBRARIES})
