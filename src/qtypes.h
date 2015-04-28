// src/qtypes.h – Define custom QML types
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QObject>
#include <QtQml>

#include <QtGui>

#include <algorithm>

#include "random.h"
#include "setup.h"
#include "utils.h"

namespace plstim
{
class Error : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString title READ title NOTIFY titleChanged)
  Q_PROPERTY (QString description READ description NOTIFY descriptionChanged)

public:
  Error (const QString& title="", const QString& description="",
	 QObject* parent=nullptr)
    : QObject (parent)
    , m_title (title)
    , m_description (description)
  {}

  QString title () const
  { return m_title; }

  QString description () const
  { return m_description; }

protected:
  QString m_title;
  QString m_description;

signals:
  void titleChanged (const QString& title);
  void descriptionChanged (const QString& description);
};


class Subject : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name)
public:
  Subject (QObject* parent=nullptr)
    : QObject (parent)
  { }

  Subject (const QString& name, QObject* parent=nullptr)
    : QObject (parent)
    , m_name (name)
  { }

  QString name () const
  { return m_name; }
protected:
  QString m_name;
};

class Parameter : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name NOTIFY nameChanged)
  Q_PROPERTY (double value READ value NOTIFY valueChanged)
  Q_PROPERTY (QString unit READ unit NOTIFY unitChanged)
public:
  Parameter (const QString& name, double value, const QString& unit)
    : m_name (name), m_value (value), m_unit (unit)
  { }

  QString name () const { return m_name; }
  double value () const { return m_value; }
  QString unit () const { return m_unit; }
protected:
  QString m_name;
  double m_value;
  QString m_unit;
signals:
  void nameChanged (const QString& name);
  void valueChanged (double value);
  void unitChanged (const QString& unit);
};

class Vector : public QObject
{
  Q_OBJECT
  Q_PROPERTY (int length READ length WRITE setLength)

public:
  Vector (QObject* parent=nullptr)
    : QObject (parent), m_data (nullptr)
  {}

  ~Vector ()
  {
    if (m_data) delete m_data;
  }

  Q_INVOKABLE float at (int index) const
  {
    if (index < 0 || index >= m_length) {
      qDebug () << "out of range Vector index" << index << "but" << m_length;
      return 0;
    }
    return m_data[index];
  }

  Q_INVOKABLE void set (int index, float value)
  {
    if (index < 0 || index >= m_length)
      qDebug () << "out of range Vector index" << index << "but" << m_length;
    else
      m_data[index] = value;
  }

  Q_INVOKABLE void set (const QList<QVariant>& values)
  {
    if (values.size () != m_length) {
      qCritical () << "error: source array size not matching target vector length";
      return;
    }

    for (int i = 0; i < m_length; i++) {
      bool ok;
      float val = values.at (i).toFloat (&ok);
      if (! ok)
	qCritical () << "error: invalid source array type";
      m_data[i] = val;
    }
  }

  Q_INVOKABLE void linear (float from=0, float step=1)
  {
    for (int i = 0; i < m_length; i++) {
      m_data[i] = from;
      from += step;
    }
  }

  Q_INVOKABLE void shuffle ()
  {
    std::random_shuffle (m_data, m_data+m_length);
  }

  int length () const
  { return m_length; }

  void setLength (int l)
  {
    if (m_data) {
      qDebug () << "cannot resize an existing Vector";
      return;
    }
    m_length = l;
    m_data = new float[m_length];
  }

protected:
  int m_length;
  float* m_data;
};


class PainterPath : public QObject
{
  Q_OBJECT

public:
  PainterPath (QObject* parent=nullptr)
    : QObject (parent)
  {
    m_path = new QPainterPath;
  }

  ~PainterPath ()
  { delete m_path; }

  Q_INVOKABLE void clear ()
  {
    delete m_path;
    m_path = new QPainterPath;
  }

  Q_INVOKABLE void addEllipse (float x, float y, float width, float height)
  {
    m_path->addEllipse (x, y, width, height);
  }

  Q_INVOKABLE void addRect (float x, float y, float width, float height)
  {
    m_path->addRect (x, y, width, height);
  }

  const QPainterPath* path () const
  {
    return m_path;
  }
protected:
  QPainterPath* m_path;
};


class Pen : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QColor color READ color WRITE setColor)

public:
  Pen (QObject* parent=nullptr)
    : QObject (parent)
  {}

  QColor color () const
  { return m_pen.color (); }

  void setColor (const QColor& color)
  { m_pen.setColor (color); }

  Q_INVOKABLE void setWidthF (float width)
  { m_pen.setWidthF (width); }

  const QPen* pen () const
  { return &m_pen; }

protected:

  QPen m_pen;
};


class Painter : public QObject
{
  Q_OBJECT

public:
  Painter (QPainter& painter, QObject* parent=nullptr)
    : QObject (parent), m_painter (painter)
  {}

  Q_INVOKABLE STACK_ALIGNED void drawEllipse (int x, int y, int width, int height)
  {
    m_painter.drawEllipse (x, y, width, height);
  }

  Q_INVOKABLE STACK_ALIGNED void drawLine (int x1, int y1, int x2, int y2)
  {
    m_painter.drawLine (x1, y1, x2, y2);
  }

  Q_INVOKABLE STACK_ALIGNED void drawText (int x, int y, const QString& text)
  {
    m_painter.drawText (x, y, text);
  }

  Q_INVOKABLE STACK_ALIGNED void drawText (int x, int y, int w, int h, int flags, const QString& text)
  {
    m_painter.drawText (x, y, w, h, flags, text);
  }

  Q_INVOKABLE STACK_ALIGNED void drawPath (PainterPath* path)
  {
    //if (path.canConvert<const PainterPath&> ())
    //PainterPath& p = path.value<PainterPath&> ();
    //const PainterPath* p = qobject_cast<const PainterPath*> (&path);
    //qDebug () << "GOT A FUCKING PATH at" << hex << (long) path;
    m_painter.drawPath (*(path->path ()));
  }

  Q_INVOKABLE STACK_ALIGNED void fillRect (int x, int y, int width, int height, const QColor& color)
  {
    m_painter.fillRect (x, y, width, height, color);
  }

  Q_INVOKABLE void setBrush (const QColor& color)
  {
    m_painter.setBrush (color);
  }

  Q_INVOKABLE void setPen ()
  {
    m_painter.setPen (Qt::NoPen);
  }

  Q_INVOKABLE void setPen (const QColor& color)
  {
    m_painter.setPen (color);
  }

  Q_INVOKABLE void setPen (const QPen& pen)
  {
    m_painter.setPen (pen);
  }

protected:
  QPainter& m_painter;
};


QString keyToString (int k);
int stringToKey (const QString& s);

class Page : public QObject
{
  Q_OBJECT
  Q_ENUMS (PaintTime)
  Q_PROPERTY (QString name READ name WRITE setName)
  Q_PROPERTY (bool last READ last WRITE setLast)
  Q_PROPERTY (int duration READ duration WRITE setDuration)
  Q_PROPERTY (int frameCount READ frameCount WRITE setFrameCount)
  Q_PROPERTY (bool animated READ animated WRITE setAnimated)
  Q_PROPERTY (PaintTime paintTime READ paintTime WRITE setPaintTime)
  Q_PROPERTY (bool waitKey READ waitKey WRITE setWaitKey)
  Q_PROPERTY (QStringList acceptedKeys READ acceptedKeys WRITE setAcceptedKeys)
#ifdef HAVE_EYELINK
  Q_PROPERTY (int fixation READ fixation WRITE setFixation)
#endif
#ifdef HAVE_POWERMATE
  Q_PROPERTY (bool waitRotation READ waitRotation WRITE setWaitRotation)
#endif // HAVE_POWERMATE

public:
  enum PaintTime
    {
      EXPERIMENT,
      TRIAL,
      ON_SHOW,
      MANUAL
    };

  Page (QObject* parent=nullptr)
    : QObject (parent)
    , m_last (false)
    , m_duration (0), m_frameCount (0)
    , m_animated (false), m_paintTime (EXPERIMENT)
    , m_waitKey (true)
#ifdef HAVE_EYELINK
    , m_fixation (0)
#endif
#ifdef HAVE_POWERMATE
    , m_waitRotation (false)
#endif // HAVE_POWERMATE
  {}

  QString name () const
  { return m_name; }

  void setName (const QString& newName)
  { m_name = newName; }

  int duration () const
  { return m_duration; }

  void setDuration (int newDuration)
  {
    m_duration = newDuration;
    m_waitKey = newDuration <= 0;
  }

  bool last () const
  { return m_last; }

  void setLast (bool last)
  { m_last = last; }

#ifdef HAVE_EYELINK
  int fixation () const
  { return m_fixation; }

  void setFixation (int fix)
  { m_fixation = fix; }
#endif

  int frameCount () const
  { return m_frameCount; }

  void setFrameCount (int count)
  { m_frameCount = count; }

  bool animated () const
  { return m_animated; }

  void setAnimated (bool anim)
  {
    m_animated = anim;
    if (anim) {
      m_paintTime = TRIAL;
      m_waitKey = false;
    }
  }

  PaintTime paintTime () const
  { return m_paintTime; }

  void setPaintTime (PaintTime time)
  { m_paintTime = time; }

  bool waitKey () const
  { return m_waitKey; }

  void setWaitKey (bool wait)
  { m_waitKey = wait; }

  bool acceptAnyKey () const
  { return m_acceptedKeys.isEmpty (); }

  bool acceptKey (int key) const
  {
    return m_acceptedKeys.contains (key);
  }

  QStringList
  acceptedKeys () const
  {
    QStringList l;
    for (auto k : m_acceptedKeys)
      l << keyToString (k);
    return l;
  }

  void setAcceptedKeys (const QStringList& keyList)
  {
    qDebug () << "Trying to set accepted keys from" << keyList;
    m_acceptedKeys.clear ();

    for (auto k : keyList)
      m_acceptedKeys << stringToKey (k);
    qDebug () << " accepted keys are now: " << m_acceptedKeys;
    //m_acceptedKeys = keyList;
  }

#ifdef HAVE_POWERMATE
  bool waitRotation () const
  { return m_waitRotation; }

  void setWaitRotation (bool wait)
  {
    m_waitKey = false;
    m_waitRotation = wait;
  }
#endif // HAVE_POWERMATE

protected:
  QString m_name;
  bool m_last;
  int m_duration;
  int m_frameCount;
  bool m_animated;
  PaintTime m_paintTime;
  bool m_waitKey;
  QSet<int> m_acceptedKeys;
#ifdef HAVE_EYELINK
  int m_fixation;
#endif
#ifdef HAVE_POWERMATE
  bool m_waitRotation;
#endif // HAVE_POWERMATE

signals:
  void showPage (Page* page);
  void paint (plstim::Painter* painter, int frameNumber);
  void keyPress (const QString& key);
#ifdef HAVE_POWERMATE
  void rotation (int step);
#endif // HAVE_POWERMATE
};

class Experiment : public QObject
{
  Q_OBJECT
  Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY (int trialCount READ trialCount WRITE setTrialCount NOTIFY trialCountChanged)
  Q_PROPERTY (float size READ size WRITE setSize)
  //Q_PROPERTY (int distance READ distance WRITE setDistance)
  //Q_PROPERTY (float refreshRate READ refreshRate WRITE setRefreshRate)
  Q_PROPERTY (float swapInterval READ swapInterval WRITE setSwapInterval)
  Q_PROPERTY (int textureSize READ textureSize WRITE setTextureSize NOTIFY textureSizeChanged)
  Q_PROPERTY (QColor background READ background WRITE setBackground)
  Q_PROPERTY (QQmlListProperty<plstim::Page> pages READ pages)
  Q_PROPERTY (QVariantMap trialParameters READ trialParameters WRITE setTrialParameters)
  Q_PROPERTY (QVariantMap subjectParameters READ subjectParameters WRITE setSubjectParameters)
  Q_PROPERTY (QVariantList modules READ modules WRITE setModules)
  Q_CLASSINFO ("DefaultProperty", "pages")
    
  public:
  Experiment (QObject* parent=nullptr)
  : QObject (parent)
    , m_swapInterval (1)
    , m_setup (nullptr)
  {
    // Initialise the random number generator
    RandomDevSeedSequence rdss;
    m_twister.seed(rdss);
  }

  int trialCount () const
  { return m_trialCount; }

  void setSize (float sz)
  { m_size = sz; }

  float size () const
  { return m_size; }

  void setTrialCount (int count)
  { m_trialCount = count; }

  QColor background () const
  { return m_background; }

  void setBackground (const QColor& color)
  { m_background = color; }

  QQmlListProperty<plstim::Page> pages ()
  { return QQmlListProperty<Page> (this, m_pages); }

  int pageCount () const
  { return m_pages.count (); }

  plstim::Page* page (int index)
  { return m_pages.at (index); }

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
    if (! m_setup) return 0;
    // Avoid division by zero on missing information
    if (! m_setup->physicalWidth () || ! m_setup->physicalHeight ()) {
      qDebug () << "No physical size for the setup!";
      return 0;
    }

    int distance = m_setup->distance ();
    float hres = static_cast<float> (m_setup->horizontalResolution ()) / m_setup->physicalWidth ();
    float vres = static_cast<float> (m_setup->verticalResolution ()) / m_setup->physicalHeight ();

    return distance*tan (radians (dst)/2)*(hres+vres);
  }

  Q_INVOKABLE float degreesPerSecondToPixelsPerFrame (float speed) const
  {
    float rate = m_setup ? m_setup->refreshRate () : 0;
    return degreesToPixels (speed/rate*m_swapInterval);
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

  void setSetup (Setup* setup)
  { m_setup = setup; }

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
  QColor m_background;
  QList<plstim::Page*> m_pages;
  QVariantMap m_trialParameters;
  QVariantMap m_subjectParameters;
  QVariantList m_modules;
  Setup* m_setup;

signals:
  void newTrial ();
  void setupUpdated ();
  void nameChanged (const QString&);
  void trialCountChanged (int);
  void textureSizeChanged (int);
};

} // namespace plstim

// Local Variables:
// mode: c++
// End:
