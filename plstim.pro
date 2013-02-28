CONFIG += qt debug -release -eyelink powermate
QT += core gui network qml quick
SOURCES += src/engine.cc src/stimwindow.cc src/utils.cc src/gui.cc src/main.cc
HEADERS += src/stimwindow.h src/plstim.h src/engine.h src/utils.h src/elcalibration.h src/qtypes.h src/gui.h src/setup.h
TARGET = plstim
QMAKE_CXXFLAGS += -std=c++0x -Woverloaded-virtual
LIBS += -lhdf5_cpp -lhdf5

eyelink {
  DEFINES += HAVE_EYELINK
  SOURCES += src/eyelink.cc
  LIBS += -leyelink_core
}

powermate {
  DEFINES += HAVE_POWERMATE
  SOURCES += src/powermate.cc
  HEADERS += src/powermate.h
}

win32 {
  INCLUDEPATH += C:/MinGW/msys/1.0/local/include
  LIBS += -L C:/MinGW/msys/1.0/local/lib
  RC_FILE += src/plstim.rc
}
