#include <iostream>
using namespace std;

#include "qswrappers.h"
using namespace plstim;

#include <QScriptEngine>


Q_DECLARE_METATYPE (QColor)
Q_DECLARE_METATYPE (QColor*)
Q_DECLARE_METATYPE (QPainter*)


ColorPrototype::ColorPrototype (QObject* parent)
  : QObject (parent)
{
}


PainterPrototype::PainterPrototype (QObject* parent)
  : QObject (parent)
{
}

QString
PainterPrototype::toString () const
{
  return QString ("QPainter");
}

void
PainterPrototype::fillRect (int x, int y, int width, int height, QColor* col)
{
  cout << "[QS:2] QPainter::fillRect()" << endl;
  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());

  painter->fillRect (x, y, width, height, *col);
}

