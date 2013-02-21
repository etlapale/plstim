CONFIG += qt debug -release -eyelink powermate
QT += core gui network widgets
SOURCES += src/qexperiment.cc src/stimwindow.cc src/messagebox.cc src/utils.cc src/main.cc
HEADERS += src/stimwindow.h src/messagebox.h src/plstim.h src/qexperiment.h src/utils.h src/elcalibration.h
TARGET = plstim
QMAKE_CXXFLAGS += -std=c++0x -Wshadow -Woverloaded-virtual
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

unix {
  LIBS += -llua
}

win32 {
  INCLUDEPATH += "C:/Program Files/Lua/5.1/include"
  INCLUDEPATH += C:/MinGW/msys/1.0/local/include
  LIBS += -L "C:/Program Files/Lua/5.1/lib" -llua51
  LIBS += -L C:/MinGW/msys/1.0/local/lib
  RC_FILE += src/plstim.rc
}
