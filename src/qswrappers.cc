#include <unistd.h>

#include <iostream>
using namespace std;

#include "qswrappers.h"
using namespace plstim;

#include <QScriptEngine>


Q_DECLARE_METATYPE (uniform_int_distribution<int>*)
Q_DECLARE_METATYPE (QColor)
Q_DECLARE_METATYPE (QColor*)
Q_DECLARE_METATYPE (QPen)
Q_DECLARE_METATYPE (QPen*)
Q_DECLARE_METATYPE (QPainterPath)
Q_DECLARE_METATYPE (QPainterPath*)
Q_DECLARE_METATYPE (QPainter*)


UniformIntDistributionPrototype::UniformIntDistributionPrototype (QObject* parent)
  : QObject (parent)
{
  twister.seed (time (NULL));
}

QString
UniformIntDistributionPrototype::toString () const
{
  return QString ("UniformIntDistribution");
}

int
UniformIntDistributionPrototype::random ()
{
  auto dist = qscriptvalue_cast<uniform_int_distribution<int>*> (thisObject ());
  return (*dist) (twister);
}


ColorPrototype::ColorPrototype (QObject* parent)
  : QObject (parent)
{
}

PenPrototype::PenPrototype (QObject* parent)
  : QObject (parent)
{
}

QString
PenPrototype::toString () const
{
  return QString ("QPen");
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
PainterPrototype::drawLine (int x1, int y1, int x2, int y2)
{
  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());
  painter->drawLine (x1, y1, x2, y2);
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
  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());
  painter->setBrush (col);
}

void
PainterPrototype::setPen (const QPen& pen)
{
  auto painter = qscriptvalue_cast<QPainter*> (thisObject ());
  painter->setPen (pen);
}
