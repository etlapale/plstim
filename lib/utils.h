// plstim/utils.h – General stimulus utilities

#ifndef __PLSTIM_UTILS_H
#define __PLSTIM_UTILS_H

#include <cmath>

namespace plstim
{
  bool initialise ();

  /// Convert an angle in radians to degrees
  static inline float degrees (float rads) {
    return 180 * rads / M_PI;
  }

  /// Convert an angle in degrees to radians
  static inline float radians (float degs) {
    return M_PI * degs / 180;
  }


  /// Convert a distance in seconds to degrees of visual field.
  static inline float
  sec2deg (float dst)
  {
    return dst / 60;
  }
}

#endif	// __PLSTIM_UTILS_H

// vim:filetype=cpp:
