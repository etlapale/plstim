#ifndef __POWERMATE_WATCHER_H
#define __POWERMATE_WATCHER_H

#include <QtCore>


class PowerMateEvent : public QEvent
{
public:
  static QEvent::Type EventType;
public:
  int step;
  PowerMateEvent (int step);
};

class PowerMateWatcher : public QObject
{
  Q_OBJECT
public slots:
  void watch ();
};

#endif // __POWERMATE_WATCHER_H
