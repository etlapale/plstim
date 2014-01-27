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

}
