#ifndef __PLSTIM_QSWRAPPERS_H
#define __PLSTIM_QSWRAPPERS_H


#include <QPainter>
#include <QScriptable>


namespace plstim {

  class ColorPrototype : public QObject, public QScriptable
  {
    Q_OBJECT
  public:
    ColorPrototype (QObject* parent = NULL);
  };

  class PenPrototype : public QObject, public QScriptable
  {
    Q_OBJECT
  public:
    PenPrototype (QObject* parent = NULL);
  public slots:
    QString toString () const;
  };

  class PainterPathPrototype : public QObject, public QScriptable
  {
    Q_OBJECT
  public:
    PainterPathPrototype (QObject* parent = NULL);
  public slots:
    QString toString () const;

    void addRect (int x, int y, int width, int height);
    void addEllipse (int x, int y, int width, int height);
  };

  class PainterPrototype : public QObject, public QScriptable
  {
    Q_OBJECT
  public:
    PainterPrototype (QObject* parent = NULL);
  public slots:
    QString toString () const;

    void setBrush (const QColor& col);
    void setPen (const QPen& pen);

    void drawEllipse (int x, int y, int width, int height);
    void drawPath (QPainterPath* path);
    void fillRect (int x, int y, int width, int height, QColor col);
  };

}

#endif
