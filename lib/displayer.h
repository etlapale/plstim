// lib/displayer.h – Base class for stimulus displayers
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtGui>

namespace plstim
{

/**
 * Abstract base class for stimulus displayers.
 * 
 * Displayers can present the subject with either fixed or animated
 * frames. Each frame and each serie of animated frame is given a
 * unique name.
 */
class Displayer
{
public:
  /// Define the content of a fixed frame.
  virtual void addFixedFrame(const QString& name, const QImage& img) = 0;
  /// Append a single frame to an animated series.
  virtual void addAnimatedFrame(const QString& name, const QImage& img) = 0;

  virtual void showFixedFrame(const QString& name) = 0;
  virtual void showAnimatedFrames(const QString& name) = 0;

  /// Remove all frames in an animated series.
  virtual void deleteAnimatedFrames(const QString& name) = 0;
  /// Destroy all frames.
  virtual void clear() = 0;

  virtual void setTextureSize(int width, int height) = 0;

  /// Show the displayer in normal (fullscreen) mode.
  virtual void begin() = 0;
  /// Show the displayer in inlined mode.
  virtual void beginInline() = 0;
  /// Hide the displayer.
  virtual void end() = 0;

  /// Associated screen.
  virtual QScreen* displayScreen() = 0;

signals:
  virtual void keyPressed(QKeyEvent* event) = 0;
  /// Sent when the displayer becomes visible.
  virtual void exposed() = 0;
};

} // namespace plstim

// Local Variables:
// mode: c++
// End:
