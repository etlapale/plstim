// plstim/setup.h – Manage hardware setup configurations

#ifndef __PLSTIM_SETUP_H
#define __PLSTIM_SETUP_H

#include <QObject>


namespace plstim
{
class Setup : public QObject
{
    Q_OBJECT
    Q_PROPERTY (QString name READ name WRITE setName)
    Q_PROPERTY (int horizontalOffset READ horizontalOffset WRITE setHorizontalOffset)
    Q_PROPERTY (int verticalOffset READ verticalOffset WRITE setVerticalOffset)

public:
    Setup (QObject* parent=NULL)
	: QObject (parent)
    {}

    QString name () const
    { return m_name; }

    void setName (const QString& name)
    { m_name = name; }

    int horizontalOffset () const
    { return m_horizontalOffset; }

    void setHorizontalOffset (int offset)
    { m_horizontalOffset = offset; }

    int verticalOffset () const
    { return m_verticalOffset; }

    void setVerticalOffset (int offset)
    { m_verticalOffset = offset; }

protected:
    QString m_name;
    int m_horizontalOffset;
    int m_verticalOffset;

#if 0
    /// Load a configuration file
    bool load (const std::string& filename=default_source);

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
