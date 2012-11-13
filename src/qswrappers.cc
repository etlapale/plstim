#include <iostream>
using namespace std;

#include "qswrappers.h"
using namespace plstim;

#include <QScriptEngine>


Q_DECLARE_METATYPE (QColor)
Q_DECLARE_METATYPE (QColor*)
Q_DECLARE_METATYPE (QPainterPath)
Q_DECLARE_METATYPE (QPainterPath*)
Q_DECLARE_METATYPE (QPainter*)


ColorPrototype::ColorPrototype (QObject* parent)
  : QObject (parent)
{
}

PainterPathPrototype::PainterPathPrototype (QObject* parent)
  : QObject (parent)
{
}

QString
PainterPathPrototype::toString () const
{
  return QString ("QPainterPath");
}

void
PainterPathPrototype::addRect (int x, int y, int width, int height)
{
  auto path = qscriptvalue_cast<QPainterPath*> (thisObject ());
  path->addRect (x, y, width, height);
}

void
PainterPathPrototype::addEllipse (int x, int y, int width, int height)
{
  auto path = qscriptvalue_cast<QPainterPath*> (thisObject ());
  path->addEllipse (x, y, width, height);
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
PainterPrototype::fillRect (int x, int y, int width, int height, QColor col)
{
  cout << "[QS] QPainter::fillRect()" << endl;
  //cout << "color is: " << col->red () << "-" << col->green () << "-" << col->blue () << endl;
  cout << "  color is: " << col.red () << "-" << col.green () << "-" << col.blue () << endl;
  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());

  painter->fillRect (x, y, width, height, col);
}

void
PainterPrototype::drawEllipse (int x, int y, int width, int height)
{
  cout << "[QS] QPainter::drawEllipse()" << endl;

  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());
  painter->drawEllipse (x, y, width, height);
}

void
PainterPrototype::drawPath (QPainterPath* path)
{
  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());
  painter->drawPath (*path);
}

void
PainterPrototype::setBrush (const QColor& col)
{
  cout << "[QS] QPainter::setBrush ()" << endl;
  cout << "  color is: " << col.red () << "-" << col.green () << "-" << col.blue () << endl;

  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());

  painter->setBrush (col);
}
