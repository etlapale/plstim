// lib/experiment.cc – QML experiment loader
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#include "experiment.h"

namespace plstim
{

QQmlComponent* load_experiment(QQmlEngine* engine,
			       const QString& path, const QString& base_dir)
{
  qDebug() << "Trying to load" << path << "from" << base_dir;
  
  QFileInfo file_info(path);
  
  // Use absolute paths
  if (! file_info.isAbsolute())
    file_info.setFile (base_dir + QDir::separator() + file_info.filePath());

  return load_experiment(engine, QUrl::fromLocalFile(file_info.filePath()));
}

QQmlComponent* load_experiment(QQmlEngine* engine, const QUrl& url)
{
  auto comp = new QQmlComponent(engine, url);
  return comp;
}

} // namespace plstim
