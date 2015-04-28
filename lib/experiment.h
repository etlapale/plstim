// lib/experiment.h – QML experiment loader
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtCore>
#include <QtQml>

namespace plstim
{

/// Load an experiment from a file.
QQmlComponent* load_experiment(QQmlEngine* engine,
			       const QString& path,
			       const QString& base_dir="");

/// Load an experiment from an URL.
QQmlComponent* load_experiment(QQmlEngine* engine, const QUrl& url);
  
} // namespace plstim

// Local Variables:
// mode: c++
// End:
