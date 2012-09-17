// plstim/setup.h – Manage hardware setup configurations

#ifndef __PLSTIM_SETUP_H
#define __PLSTIM_SETUP_H

#include <cmath>


namespace plstim
{
  /// Convert an angle in radians to degrees
  static inline float degrees (float rads) {
    return 180 * rads / M_PI;
  }

  /// Convert an angle in degrees to radians
  static inline float radians (float degs) {
    return M_PI * degs / 180;
  }

  class Setup
  {
  public:
    /// Sample default configuration file name
    static const std::string default_source;

  public:
    /// Monitor refresh rate
    float refresh;
    /// Distance between the eye and the monitor
    float distance;
    /// Display resolution in pixels
    float resolution[2];
    /// Physical size of the display
    float size[2];
    /// Luminance range available on the display
    float luminance[2];

    /// Computed resolution [px/mm]
    float px_mm;

  public:
    Setup ();

    /// Load a configuration file
    bool load (const std::string& filename=default_source);

    /// Convert a pixel distance to degrees
    float pix2deg (float dst) const {
      return degrees (2 * atan2 (dst, 2*distance*px_mm));
    }

    /// Convert a degrees distance to pixels
    float deg2pix (float dst) const {
      return 2*distance*tan(radians(dst)/2)*px_mm;
    }

    /// Convert a duration in seconds to frames
    float sec2frm (float dur) const {
      cout << dur << " " << refresh << endl;
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
  };
}

#endif

// vim:filetype=cpp:
