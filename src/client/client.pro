CONFIG += qt -debug release
QT += core gui network qml quick
SOURCES += client.cc
HEADERS += client.h
TARGET = plclient
QMAKE_CXXFLAGS += -std=c++0x -Woverloaded-virtual
