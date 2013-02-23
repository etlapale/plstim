#ifndef __POWERMATE_WATCHER_H
#define __POWERMATE_WATCHER_H

#include <QtCore>


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
  PowerMateEvent (QEvent::Type eventType);
};

class PowerMateWatcher : public QObject
{
  Q_OBJECT
public slots:
  void watch ();
};

#endif // __POWERMATE_WATCHER_H
