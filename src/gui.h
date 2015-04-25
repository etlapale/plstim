// src/gui.h – Controller GUI
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtQuick>
#include "engine.h"


namespace plstim
{
class GUI : public QObject
{
    Q_OBJECT
public:
    GUI (const QUrl& uipath);
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
