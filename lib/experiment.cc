// lib/experiment.cc – Experiment loader
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#include "experiment.h"

namespace plstim
{

std::pair<QQmlComponent*,QObject*> load_experiment(QQmlEngine* engine,
						   const QString& path,
						   const QString& base_dir)
{
  qDebug() << "Trying to load" << path << "from" << base_dir;
  
  QFileInfo file_info(path);
  
  // Use absolute paths
  if (! file_info.isAbsolute())
    file_info.setFile (base_dir + QDir::separator() + file_info.filePath());

  return load_experiment(engine, QUrl::fromLocalFile(file_info.filePath()));
}

std::pair<QQmlComponent*,QObject*> load_experiment(QQmlEngine* engine,
						   const QUrl& url)
{
  using std::make_pair;
  
  // Load the experiment
  auto comp = new QQmlComponent(engine, url);
  if (comp->isError())
    return make_pair(comp, nullptr);

  // Instanciate the component
  auto xp = comp->create();
  if (xp == nullptr || comp->isError())
    return make_pair(comp, nullptr);
  
  return make_pair(comp,xp);
}

} // namespace plstim
