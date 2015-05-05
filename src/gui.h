// src/gui.h – Controller GUI
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtQuick>
#include "../lib/engine.h"
#include "stimwindow.h"


namespace plstim
{
  
class GUI : public QObject
{
  Q_OBJECT
public:

  /// Screen on which to display the stimuli.
  static QScreen* displayScreen();
  
  GUI(const QUrl& uipath);
  virtual ~GUI();

  plstim::Engine* engine()
  { return m_engine; }

public slots:
  /// Load the given experiment from an URL.
  void loadExperiment(const QUrl& url);
  
protected:
  QQmlApplicationEngine m_ui_engine;
  plstim::Engine* m_engine;
  StimWindow* m_displayer;
  QStringList m_subjectList;
			   
			     
protected slots:
  void subjectSelected (int index);
};
 
}

// Local Variables:
// mode: c++
// End:
