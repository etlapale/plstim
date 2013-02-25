#ifndef __PLSTIM_QTYPES_H
#define __PLSTIM_QTYPES_H

#include <QObject>
#include <QtQml>

#include <QtGui>

#include "utils.h"

namespace plstim
{

class Vector : public QObject
{
    Q_OBJECT
    Q_PROPERTY (int length READ length WRITE setLength)

public:
    Vector (QObject* parent=NULL)
	: QObject (parent), m_data (NULL)
    {}

    ~Vector ()
    {
	if (m_data) delete m_data;
    }

    Q_INVOKABLE float at (int index) const
    {
	if (index < 0 || index >= m_length) {
	    qDebug () << "out of range Vector index";
	    return 0;
	}
	return m_data[index];
    }

    Q_INVOKABLE void set (int index, float value)
    {
	if (index < 0 || index >= m_length)
	    qDebug () << "out of range Vector index";
	else
	    m_data[index] = value;
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
    PainterPath (QObject* parent=NULL)
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
    Pen (QObject* parent=NULL)
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
    Painter (QPainter& painter, QObject* parent=NULL)
	: QObject (parent), m_painter (painter)
    {}

    Q_INVOKABLE void drawEllipse (int x, int y, int width, int height)
    {
	m_painter.drawEllipse (x, y, width, height);
    }

    Q_INVOKABLE void drawLine (int x1, int y1, int x2, int y2)
    {
	m_painter.drawLine (x1, y1, x2, y2);
    }

    //Q_INVOKABLE void drawPath (const PainterPath& path)
    Q_INVOKABLE void drawPath (PainterPath* path)
    {
	//if (path.canConvert<const PainterPath&> ())
	//PainterPath& p = path.value<PainterPath&> ();
	//const PainterPath* p = qobject_cast<const PainterPath*> (&path);
	//qDebug () << "GOT A FUCKING PATH at" << hex << (long) path;
	m_painter.drawPath (*(path->path ()));
    }

    Q_INVOKABLE void setBrush (const QColor& color)
    {
	m_painter.setBrush (color);
    }

    Q_INVOKABLE void setPen ()
    {
	m_painter.setPen (Qt::NoPen);
    }

    Q_INVOKABLE void setPen (Pen* pen)
    {
	m_painter.setPen (*(pen->pen ()));
    }

protected:
    QPainter& m_painter;
};



class Page : public QObject
{
    Q_OBJECT
    Q_ENUMS (PaintTime)
    Q_PROPERTY (QString name READ name WRITE setName)
    Q_PROPERTY (int duration READ duration WRITE setDuration)
    Q_PROPERTY (int frameCount READ frameCount WRITE setFrameCount)
    Q_PROPERTY (bool animated READ animated WRITE setAnimated)
    Q_PROPERTY (PaintTime paintTime READ paintTime WRITE setPaintTime)
    Q_PROPERTY (bool waitKey READ waitKey WRITE setWaitKey)
#ifdef HAVE_POWERMATE
    Q_PROPERTY (bool waitRotation READ waitRotation WRITE setWaitRotation)
#endif // HAVE_POWERMATE

public:
    enum PaintTime
    {
	EXPERIMENT,
	TRIAL,
	MANUAL
    };

    Page (QObject* parent=NULL)
	: QObject (parent)
	, m_duration (0), m_frameCount (0)
	, m_animated (false), m_paintTime (EXPERIMENT)
	, m_waitKey (true)
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
	return m_acceptedKeys.isEmpty ()
	    || m_acceptedKeys.contains (key);
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
    int m_duration;
    int m_frameCount;
    bool m_animated;
    PaintTime m_paintTime;
    bool m_waitKey;
    QSet<int> m_acceptedKeys;
#ifdef HAVE_POWERMATE
    bool m_waitRotation;
#endif // HAVE_POWERMATE

signals:
    //void paint (QPainter* painter, int frameNumber);
    void paint (plstim::Painter* painter, int frameNumber);
};

class Experiment : public QObject
{
    Q_OBJECT
    Q_PROPERTY (int trialCount READ trialCount WRITE setTrialCount)
    Q_PROPERTY (float size READ size WRITE setSize)
    Q_PROPERTY (int distance READ distance WRITE setDistance)
    Q_PROPERTY (float refreshRate READ refreshRate WRITE setRefreshRate)
    Q_PROPERTY (float swapInterval READ swapInterval WRITE setSwapInterval)
    Q_PROPERTY (int textureSize READ textureSize WRITE setTextureSize)
    Q_PROPERTY (QColor background READ background WRITE setBackground)
    Q_PROPERTY (QQmlListProperty<plstim::Page> pages READ pages)
    Q_PROPERTY (QVariantMap trialParameters READ trialParameters WRITE setTrialParameters)
    Q_CLASSINFO ("DefaultProperty", "pages")
    
public:
    Experiment (QObject* parent=NULL)
	: QObject (parent),
	  m_swapInterval (1)
    {
	// Initialise the random number generator
	m_twister.seed (time (NULL));
	for (int i = 0; i < 10000; i++)
	    (void) randint (1024);
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

    float distance () const
    { return m_distance; }

    void setDistance (float dst)
    { m_distance = dst; }

    float refreshRate () const
    { return m_refreshRate; }

    void setRefreshRate (float rate)
    { m_refreshRate = rate; }

    int swapInterval () const
    { return m_swapInterval; }

    void setSwapInterval (int interval)
    { m_swapInterval = interval; }

    int textureSize () const
    { return m_textureSize; }

    void setTextureSize (int size)
    { m_textureSize = size; }

    float verticalResolution () const
    { return m_verticalResolution; }

    void setVerticalResolution (float vres)
    { m_verticalResolution = vres; }

    float horizontalResolution () const
    { return m_horizontalResolution; }

    void setHorizontalResolution (float hres)
    { m_horizontalResolution = hres; }

    Q_INVOKABLE int randint (int maxval)
    {
	std::uniform_int_distribution<int> distrib (0, maxval);
	return distrib (m_twister);
    }

    Q_INVOKABLE bool randbool ()
    {
	std::uniform_int_distribution<int> distrib (0, 1);
	return (bool) distrib (m_twister);
    }

    Q_INVOKABLE float random ()
    {
	std::uniform_real_distribution<float> distrib (0, 1);
	return distrib (m_twister);
    }

    /// Convert a distance from degrees to pixels
    Q_INVOKABLE float degreesToPixels (float dst) const
    {
	float avgRes = (m_horizontalResolution + m_verticalResolution) / 2;
	return 2*m_distance*tan (radians (dst)/2)*avgRes;
    }

    Q_INVOKABLE float degreesPerSecondToPixelsPerFrame (float speed) const
    {
	return degreesToPixels (speed/m_refreshRate*m_swapInterval);
    }

    const QVariantMap& trialParameters () const
    { return m_trialParameters; }

    void setTrialParameters (const QVariantMap& params)
    { m_trialParameters = params; }

protected:
    /// Pseudo random number generator
    std::mt19937 m_twister;

    int m_trialCount;
    float m_size;
    int m_textureSize;
    float m_distance;
    float m_verticalResolution;
    float m_horizontalResolution;
    float m_refreshRate;
    float m_swapInterval;
    QColor m_background;
    QList<plstim::Page*> m_pages;
    QVariantMap m_trialParameters;

signals:
    void newTrial ();
    void setupUpdated ();
};

}

#endif // __PLSTIM_QTYPES_H
