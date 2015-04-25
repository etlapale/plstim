// src/qtypes.h – Define custom QML types
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#include "qtypes.h"

namespace plstim
{

QString
keyToString (int k)
{
    if (k == Qt::Key_Left)
        return "Left";
    else if (k == Qt::Key_Right)
        return "Right";
    else if (k == Qt::Key_Up)
        return "Up";
    else if (k == Qt::Key_Down)
        return "Down";
    else
        return "";
}


int
stringToKey (const QString& s)
{
  if (s == "Left")
      return Qt::Key_Left;
  else if (s == "Right")
      return Qt::Key_Right;
  else if (s == "Up")
      return Qt::Key_Up;
  else if (s == "Down")
      return Qt::Key_Down;
  else
      return 0;
}

} // namespace plstim
