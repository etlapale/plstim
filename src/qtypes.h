#ifndef __PLSTIM_QTYPES_H
#define __PLSTIM_QTYPES_H

#include <QObject>
#include <QtQml>
#include <QColor>

#include "utils.h"

namespace plstim
{

class QPage : public QObject
{
    Q_OBJECT
    Q_PROPERTY (QString name READ name WRITE setName)
    Q_PROPERTY (int duration READ duration WRITE setDuration)
    Q_PROPERTY (bool animated READ animated WRITE setAnimated)
#ifdef HAVE_POWERMATE
    Q_PROPERTY (bool waitRotation READ waitRotation WRITE setWaitRotation)
#endif // HAVE_POWERMATE

public:
    QPage (QObject* parent=NULL)
	: QObject (parent)
    {}

    QString name () const
    { return m_name; }

    void setName (const QString& newName)
    { m_name = newName; }

    int duration () const
    { return m_duration; }

    void setDuration (int newDuration)
    { m_duration = newDuration; }

    bool animated () const
    { return m_animated; }

    void setAnimated (bool anim)
    { m_animated = anim; }

#ifdef HAVE_POWERMATE
    bool waitRotation () const
    { return m_waitRotation; }

    void setWaitRotation (bool wait)
    { m_waitRotation = wait; }
#endif // HAVE_POWERMATE

protected:
    QString m_name;
    int m_duration;
    bool m_animated;
#ifdef HAVE_POWERMATE
    bool m_waitRotation;
#endif // HAVE_POWERMATE
};

class Experiment : public QObject
{
    Q_OBJECT
    Q_PROPERTY (int trialCount READ trialCount WRITE setTrialCount)
    Q_PROPERTY (int size READ size WRITE setSize)
    Q_PROPERTY (int distance READ distance WRITE setDistance)
    //Q_PROPERTY (int distance READ distance WRITE setDistance)
    Q_PROPERTY (QColor background READ background WRITE setBackground)
    Q_PROPERTY (QQmlListProperty<plstim::QPage> pages READ pages)
    Q_CLASSINFO ("DefaultProperty", "pages")
    
public:
    Experiment (QObject* parent=NULL)
	: QObject (parent)
    {
	// Initialise the random number generator
	m_twister.seed (time (NULL));
	for (int i = 0; i < 10000; i++)
	    (void) randint (1024);
    }

    int trialCount () const
    { return m_trialCount; }

    void setSize (int sz)
    { m_size = sz; }

    int size () const
    { return m_size; }

    void setTrialCount (int count)
    { m_trialCount = count; }

    QColor background () const
    { return m_background; }

    void setBackground (const QColor& color)
    { m_background = color; }

    QQmlListProperty<plstim::QPage> pages ()
    { return QQmlListProperty<QPage> (this, m_pages); }

    int pageCount () const
    { return m_pages.count (); }

    plstim::QPage* page (int index)
    { return m_pages.at (index); }

    float distance () const
    { return m_distance; }

    void setDistance (float dst)
    { m_distance = dst; }

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

    /// Convert a distance from degrees to pixels
    Q_INVOKABLE int degreesToPixels (float dst) const
    {
	float avgRes = (m_horizontalResolution + m_verticalResolution) / 2;
	return 2*m_distance*tan (radians (dst)/2)*avgRes;
    }

protected:
    /// Pseudo random number generator
    std::mt19937 m_twister;

    int m_trialCount;
    int m_size;
    float m_distance;
    float m_verticalResolution;
    float m_horizontalResolution;
    QColor m_background;
    QList<plstim::QPage*> m_pages;

signals:
    void newTrial ();
};

}

#endif // __PLSTIM_QTYPES_H
