// lib/experiment.h – Experiment loader
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <utility>

#include <QtCore>
#include <QtQml>

namespace plstim
{

/// Load an experiment from a file.
std::pair<QQmlComponent*,QObject*> load_experiment(QQmlEngine* engine,
						   const QString& path,
						   const QString& base_dir);

/// Load an experiment from an URL.
std::pair<QQmlComponent*,QObject*> load_experiment(QQmlEngine* engine,
						   const QUrl& url);
  
} // namespace plstim

// Local Variables:
// mode: c++
// End:
