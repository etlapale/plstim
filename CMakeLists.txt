project (plstim)
cmake_minimum_required (VERSION 2.8)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -g")

# Search for Qt modules
find_package (Qt4 REQUIRED)
set ( QT_USE_QTOPENGL TRUE )
set ( QT_USE_QTNETWORK TRUE )
include ( ${QT_USE_FILE} )

# Search for Lua
find_package (Lua51 REQUIRED)
include_directories (${LUA_INCLUDE_DIR})

# Main program
set (plstim_src src/qexperiment.cc src/utils.cc)
set (plstim_hdr src/qexperiment.h src/glwidget.h)
qt4_wrap_cpp (plstim_moc ${plstim_hdr})
add_executable (plstim ${plstim_src} ${plstim_moc})
target_link_libraries (plstim ${QT_LIBRARIES} ${LUA_LIBRARIES})
