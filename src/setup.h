// plstim/setup.h – Manage hardware setup configurations

#ifndef __PLSTIM_SETUP_H
#define __PLSTIM_SETUP_H

#include <QObject>


namespace plstim
{
class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY (QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY (int horizontalResolution READ horizontalResolution WRITE setHorizontalResolution NOTIFY horizontalResolutionChanged)
    Q_PROPERTY (int verticalResolution READ verticalResolution WRITE setVerticalResolution NOTIFY verticalResolutionChanged)
    Q_PROPERTY (int distance READ distance WRITE setDistance NOTIFY distanceChanged)
    Q_PROPERTY (float refreshRate READ refreshRate WRITE setRefreshRate NOTIFY refreshRateChanged)
    Q_PROPERTY (int physicalWidth READ physicalWidth WRITE setPhysicalWidth NOTIFY physicalWidthChanged)
    Q_PROPERTY (int physicalHeight READ physicalHeight WRITE setPhysicalHeight NOTIFY physicalHeightChanged)

signals:
    void nameChanged (const QString& name);
    void horizontalResolutionChanged (int resolution);
    void verticalResolutionChanged (int resolution);
    void distanceChanged (int distance);
    void refreshRateChanged (float rate);
    void physicalWidthChanged (int width);
    void physicalHeightChanged (int height);

public:
    Setup (QObject* parent=NULL)
	: QObject (parent)
	, m_horizontalResolution (0), m_verticalResolution (0)
	, m_distance (0), m_refreshRate (0)
	, m_physicalWidth (0), m_physicalHeight (0)
    {}

    QString name () const
    { return m_name; }

    void setName (const QString& name)
    {
	m_name = name;
	emit nameChanged (name);
    }

    int horizontalResolution () const
    { return m_horizontalResolution; }

    void setHorizontalResolution (int resolution)
    {
	m_horizontalResolution = resolution;
	emit horizontalResolutionChanged (resolution);
    }

    int verticalResolution () const
    { return m_verticalResolution; }

    void setVerticalResolution (int resolution)
    {
	m_verticalResolution = resolution;
	emit verticalResolutionChanged (resolution);
    }

    int distance () const
    { return m_distance; }

    void setDistance (int distance)
    {
	m_distance = distance;
	emit distanceChanged (distance);
    }

    float refreshRate () const
    { return m_refreshRate; }

    void setRefreshRate (float rate)
    {
	m_refreshRate = rate;
	emit refreshRateChanged (rate);
    }

    int physicalWidth () const
    { return m_physicalWidth; }

    void setPhysicalWidth (int width)
    {
	m_physicalWidth = width;
	emit physicalWidthChanged (width);
    }

    int physicalHeight () const
    { return m_physicalHeight; }

    void setPhysicalHeight (int height)
    {
	m_physicalHeight = height;
	emit physicalHeightChanged (height);
    }

protected:
    QString m_name;
    int m_horizontalOffset;
    int m_verticalOffset;
    int m_horizontalResolution;
    int m_verticalResolution;
    int m_distance;
    float m_refreshRate;
    int m_physicalWidth;
    int m_physicalHeight;

#if 0
    /// Convert a pixel distance to degrees
    float pix2deg (float dst) const {
	return degrees (2 * atan2 (dst, 2*distance*px_mm));
    }

    /// Convert a duration in seconds to frames
    float sec2frm (float dur) const {
	return dur * refresh;
    }

    /// Convert a duration in frames to seconds
    float frm2sec (float dur) const {
	return dur / refresh;
    }

    /// Convert a speed in degrees/seconds to pixels/frame
    float ds2pf (float speed) const {
	return deg2pix (speed/refresh);
    }

    /// Convert a luminance value to a pixel value in [0,1]
    /// Assumes a linear luminance on the monitor
    float lum2px (float lum) const {
	return fmax (0, fmin (1,
		  (lum - luminance[0]) / (luminance[1] - luminance[0])));
    }
#endif
};
}

#endif

// vim:filetype=cpp:
