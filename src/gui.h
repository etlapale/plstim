#ifndef __PLSTIM_GUI_H
#define __PLSTIM_GUI_H

#include <QtQuick>
#include "engine.h"


namespace plstim
{
class GUI : public QObject //: public QQuickView
{
    Q_OBJECT
public:
    GUI (const QString& uipath);//QWindow* parent=NULL);
    void loadExperiment (const QString& path);

    plstim::Engine* engine ()
    { return &m_engine; }
protected:
    QQmlApplicationEngine m_ui_engine;
    plstim::Engine m_engine;
    QStringList m_subjectList;
protected slots:
    void subjectSelected (int index);
};
}

#endif // __PLSTIM_GUI_H
