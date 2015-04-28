// lib/experiment.h – QML experiment loader
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtCore>
#include <QtQml>

namespace plstim
{



class Experiment2 : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY (int trialCount READ trialCount WRITE setTrialCount NOTIFY trialCountChanged)
  Q_PROPERTY (float size READ size WRITE setSize)
  //Q_PROPERTY (int distance READ distance WRITE setDistance)
  //Q_PROPERTY (float refreshRate READ refreshRate WRITE setRefreshRate)
  Q_PROPERTY (float swapInterval READ swapInterval WRITE setSwapInterval)
  Q_PROPERTY (int textureSize READ textureSize WRITE setTextureSize NOTIFY textureSizeChanged)
  //Q_PROPERTY (QColor background READ background WRITE setBackground)
  //Q_PROPERTY (QQmlListProperty<plstim::Page> pages READ pages)
  Q_PROPERTY (QVariantMap trialParameters READ trialParameters WRITE setTrialParameters)
  Q_PROPERTY (QVariantMap subjectParameters READ subjectParameters WRITE setSubjectParameters)
  Q_PROPERTY (QVariantList modules READ modules WRITE setModules)
  Q_CLASSINFO ("DefaultProperty", "pages")
    
  public:
  Experiment2 (QObject* parent=nullptr)
  : QObject (parent)
    , m_swapInterval (1)
    //, m_setup (nullptr)
  {
    // Initialise the random number generator
    //RandomDevSeedSequence rdss;
    //m_twister.seed(rdss);
  }

  int trialCount () const
  { return m_trialCount; }

  void setSize (float sz)
  { m_size = sz; }

  float size () const
  { return m_size; }

  void setTrialCount (int count)
  { m_trialCount = count; }

  //QColor background () const
  //{ return m_background; }

  //void setBackground (const QColor& color)
  //{ m_background = color; }

  //QQmlListProperty<plstim::Page> pages ()
  //{ return QQmlListProperty<Page> (this, m_pages); }

  //int pageCount () const
  //{ return m_pages.count (); }

  //plstim::Page* page (int index)
  //{ return m_pages.at (index); }

  int swapInterval () const
  { return m_swapInterval; }

  void setSwapInterval (int interval)
  { m_swapInterval = interval; }

  int textureSize () const
  { return m_textureSize; }

  void setTextureSize (int size)
  { m_textureSize = size; }

  Q_INVOKABLE int randint (int maxval)
  {
    std::uniform_int_distribution<int> distrib (0, maxval);
    return distrib (m_twister);
  }

  Q_INVOKABLE bool randbool ()
  {
    std::uniform_int_distribution<int> distrib (0, 1);
    return distrib (m_twister);
  }

  Q_INVOKABLE float random ()
  {
    std::uniform_real_distribution<float> distrib (0, 1);
    return distrib (m_twister);
  }

  /// Convert a distance from degrees to pixels
  Q_INVOKABLE float degreesToPixels (float dst) const
  {
    /*if (! m_setup) return 0;
    // Avoid division by zero on missing information
    if (! m_setup->physicalWidth () || ! m_setup->physicalHeight ()) {
      qDebug () << "No physical size for the setup!";
      return 0;
    }

    int distance = m_setup->distance ();
    float hres = static_cast<float> (m_setup->horizontalResolution ()) / m_setup->physicalWidth ();
    float vres = static_cast<float> (m_setup->verticalResolution ()) / m_setup->physicalHeight ();

    return distance*tan (radians (dst)/2)*(hres+vres);*/

    return 0;
  }

  Q_INVOKABLE float degreesPerSecondToPixelsPerFrame (float speed) const
  {
    /*float rate = m_setup ? m_setup->refreshRate () : 0;
      return degreesToPixels (speed/rate*m_swapInterval);*/

    return 0;
  }

  const QVariantMap& trialParameters () const
  { return m_trialParameters; }

  void setTrialParameters (const QVariantMap& params)
  { m_trialParameters = params; }

  const QVariantMap& subjectParameters () const
  { return m_subjectParameters; }

  void setSubjectParameters (const QVariantMap& params)
  { m_subjectParameters = params; }

  const QVariantList& modules () const
  { return m_modules; }

  void setModules (const QVariantList& modules)
  { m_modules = modules; }

  //void setSetup (Setup* setup)
  //{ m_setup = setup; }

  QString name () const { return m_name; }

  void setName (const QString& name) { m_name = name; }

protected:
  /// Pseudo random number generator
  std::mt19937 m_twister;

  QString m_name;
  int m_trialCount;
  float m_size;
  int m_textureSize;
  float m_swapInterval;
  //QColor m_background;
  //QList<plstim::Page*> m_pages;
  QVariantMap m_trialParameters;
  QVariantMap m_subjectParameters;
  QVariantList m_modules;
  //Setup* m_setup;

signals:
  void newTrial ();
  void setupUpdated ();
  void nameChanged (const QString&);
  void trialCountChanged (int);
  void textureSizeChanged (int);
};



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
