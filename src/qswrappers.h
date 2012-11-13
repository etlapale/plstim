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

  class PainterPrototype : public QObject, public QScriptable
  {
    Q_OBJECT
  public:
    PainterPrototype (QObject* parent = NULL);
  public slots:
    QString toString () const;
    void fillRect (int x, int y, int width, int height, QColor* col);
  };

}

#endif
