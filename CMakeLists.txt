project (plstim)
cmake_minimum_required (VERSION 2.8)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")
include_directories (".")

# Search for Qt modules
find_package (Qt4 REQUIRED)
set ( QT_USE_QTOPENGL TRUE )
set ( QT_USE_QTNETWORK TRUE )
include ( ${QT_USE_FILE} )

# Shared library
set (lib_src lib/qexperiment.cc lib/utils.cc)
set (lib_hdr lib/qexperiment.h lib/glwidget.h)
qt4_wrap_cpp (lib_moc ${lib_hdr})
add_library (plstim SHARED ${lib_src} ${lib_moc})

# Experiments
set (se_src apps/simple-experiment/simple-experiment.cc)
set (se_hdr apps/simple-experiment/simple-experiment.h)
qt4_wrap_cpp (se_moc ${se_hdr})
add_executable (simple-experiment ${se_src} ${se_moc})
target_link_libraries (simple-experiment
  plstim ${QT_LIBRARIES} ${YAMLCPP_LIBRARIES})
