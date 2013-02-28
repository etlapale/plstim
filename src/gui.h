#ifndef __PLSTIM_GUI_H
#define __PLSTIM_GUI_H

#include <QtQuick>
#include "qexperiment.h"


namespace plstim
{
class GUI : public QQuickView
{
public:
    GUI (QWindow* parent=NULL);
    void loadExperiment (const QString& path);
protected:
    plstim::QExperiment m_engine;
};
}

#endif // __PLSTIM_GUI_H
