cmake_minimum_required (VERSION 2.8.9)

project (plstim)

# Remove warning on Windows
cmake_policy (SET CMP0020 NEW)

# General build setup
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-literal-suffix")
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

# Haskell stub headers
#include_directories (${CMAKE_SOURCE_DIR}/hsrc)

# Fetch GHC’s include directory
#find_program (GHC_EXECUTABLE NAMES ghc DOC "GHC compiler")
#execute_process (COMMAND ${GHC_EXECUTABLE} --print-libdir
#    OUTPUT_VARIABLE GHC_LIBDIR
#    OUTPUT_STRIP_TRAILING_WHITESPACE)
#message ("GHC libdir found at: ${GHC_LIBDIR}")
#include_directories (${GHC_LIBDIR}/include)

# GHC packages
#find_program (GHC_PKG_EXECUTABLE NAMES ghc-pkg DOC "GHC package manager")

# Fetch a GHC package info
#macro (FindGhcPackage pkg)
#  execute_process (COMMAND ${GHC_PKG_EXECUTABLE} field ${pkg} ld-options
#      OUTPUT_VARIABLE GHC_PKG_LDFLAGS)
#  string (SUBSTRING ${GHC_PKG_LDFLAGS} 12 -1 GHC_PKG_LDFLAGS)
#  string (REGEX REPLACE "[\r\n ]+" " " GHC_PKG_LDFLAGS "${GHC_PKG_LDFLAGS}")
#  # Find Haskell libs
#  execute_process (COMMAND ${GHC_PKG_EXECUTABLE} field ${pkg} hs-libraries
#      OUTPUT_VARIABLE GHC_PKG_HS_LIBS
#      OUTPUT_STRIP_TRAILING_WHITESPACE)
#  string (SUBSTRING ${GHC_PKG_HS_LIBS} 14 -1 GHC_PKG_HS_LIBS)
#  string (REGEX REPLACE "[\r\n ]+" ";" GHC_PKG_HS_LIBS "${GHC_PKG_HS_LIBS}")
#  # Find extra libs
#  execute_process (COMMAND ${GHC_PKG_EXECUTABLE} field ${pkg} extra-libraries
#      OUTPUT_VARIABLE GHC_PKG_EXTRA_LIBS
#      OUTPUT_STRIP_TRAILING_WHITESPACE)
#  string (SUBSTRING ${GHC_PKG_EXTRA_LIBS} 16 -1 GHC_PKG_EXTRA_LIBS)
#  string (REGEX REPLACE "[\r\n ]+" ";" GHC_PKG_EXTRA_LIBS "${GHC_PKG_EXTRA_LIBS}")
#  # Define the package specific variables
#  string (TOUPPER ${pkg} UPKG)
#  set (GHC_${UPKG}_LDFLAGS "${GHC_PKG_LDFLAGS}")
#  set (GHC_${UPKG}_LIBS "${GHC_PKG_HS_LIBS};${GHC_PKG_EXTRA_LIBS}")
#endmacro(FindGhcPackage)

# Find standard GHC packages
#FindGhcPackage (rts)
#FindGhcPackage (base)
#set (GHC_LDFLAGS "${GHC_RTS_LDFLAGS} ${GHC_BASE_LDFLAGS}")
#set (GHC_LIBS "${GHC_RTS_LIBS};${GHC_BASE_LIBS}")

#set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} hsrc/PlStim.o '-lHSbase-4.6.0.1' '-lHSinteger-gmp-0.5.0.0' '-lgmp' '-lHSghc-prim-0.3.0.0' ${GHC_LDFLAGS} -L${GHC_LIBDIR}/base-4.6.0.1 -L${GHC_LIBDIR}/integer-gmp-0.5.0.0 -L${GHC_LIBDIR}/ghc-prim-0.3.0.0 -L${GHC_LIBDIR}")

# Call GHC for the Haskell code
#add_custom_command (
#  SOURCE ${PROJECT_SOURCE_DIR}/hsrc/PlStim.hs
#  COMMAND ${GHC_EXECUTABLE}
#  ARGS -c ${PROJECT_SOURCE_DIR}/hsrc/PlStim.hs
#  OUTPUT ${PROJECT_BINARY_DIR}/hsrc/PlStim_stub.h)
#set_source_files_properties (${PROJECT_BINARY_DIR}/hsrc/PlStim_stub.h GENERATED)
#add_custom_target (hcode DEPENDS ${PROJECT_BINARY_DIR}/hsrc/PlStim_stub.h)

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

set (CMAKE_AUTOMOC ON)

# Main program
set (plstim_src src/engine.cc src/stimwindow.cc src/utils.cc src/gui.cc src/qtypes.cc src/setup.cc src/main.cc ${eyelink_src})
qt5_add_resources (plstim_qrc plstim.qrc)
add_executable (plstim ${plstim_src} ${plstim_qrc})
#add_dependencies(plstim hcode)
#add_dependencies(plstim hcode)
qt5_use_modules (plstim Core Gui Network Qml Quick ${eyelink_qt_modules})
#target_link_libraries (plstim ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES} ${GHC_RTS_LIBS})
target_link_libraries (plstim ${HDF5_LIBRARIES} ${OPENGL_LIBRARIES} ${EYELINK_LIBRARIES})

# vim: sw=2
