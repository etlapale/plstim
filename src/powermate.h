// src/powermate.h – Handle events from PowerMate devices
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtCore>

namespace plstim {

class PowerMateEvent : public QEvent
{
public:
  static QEvent::Type Rotation;
  static QEvent::Type ButtonPress;
  static QEvent::Type ButtonRelease;
  static QEvent::Type LED;
public:
  int step;
  int luminance;
  PowerMateEvent(QEvent::Type eventType);
};

class PowerMateWatcher : public QObject
{
  Q_OBJECT
public slots:
  void watch();
};

// namespace plstim

// Local Variables:
// mode: c++
// End:
