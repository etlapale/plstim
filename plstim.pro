CONFIG += qt debug -release eyelink network powermate
QT += core gui network qml quick
SOURCES += src/engine.cc src/stimwindow.cc src/utils.cc src/gui.cc src/main.cc
HEADERS += src/stimwindow.h src/plstim.h src/engine.h src/utils.h src/elcalibration.h src/qtypes.h src/gui.h src/setup.h
TARGET = plstim
QMAKE_CXXFLAGS += -std=c++0x -Woverloaded-virtual
LIBS += -lhdf5_cpp -lhdf5

eyelink {
    DEFINES += HAVE_EYELINK
    SOURCES += src/eyelink.cc
    QT += widgets
    LIBS += -leyelink_core
}

powermate {
    DEFINES += HAVE_POWERMATE
    SOURCES += src/powermate.cc
    HEADERS += src/powermate.h
}

network {
    DEFINES += WITH_NETWORK
    HEADERS += src/server.h
}

win32 {
    CONFIG += console
    DEFINES += HAVE_WIN32
    INCLUDEPATH += C:/MinGW/msys/1.0/include
    LIBS += -L C:/MinGW/msys/1.0/lib -lz
    eyelink {
        INCLUDEPATH += "/c/Program Files/SR Research/EyeLink/Includes/eyelink"
        LIBS += -L"/c/Program Files/SR Research/EyeLink/libs"
    }
    powermate {
        INCLUDEPATH += C:/MinGW/msys/1.0/include/w32api
        LIBS += -L/MinGW/msys/1.0/lib/w32api -lhid -lsetupapi
    }
    RC_FILE += src/plstim.rc
}
