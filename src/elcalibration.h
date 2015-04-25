// src/elcalibration.h – QtGui EyeLink calibrator
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QQueue>
#include <QGraphicsView>
#include <core_expt.h>


namespace plstim
{
  class EyeLinkCalibrator : public QGraphicsView
  {
  protected:
    QGraphicsItem* target;
  public:
    QGraphicsPixmapItem* camera;
  protected:
    float target_size;
  public:
    QGraphicsScene sc;
    QQueue<KeyInput*> key_queue;
  protected:
    virtual void keyPressEvent (QKeyEvent* event);
    virtual void keyReleaseEvent (QKeyEvent* event);
    void add_key_event (QKeyEvent* event, bool pressed);
  public:
    EyeLinkCalibrator (QWidget* parent=NULL);
    void add_target (float x, float y);
    void erase_target ();
    void remove_camera ();
    void clear ();
    void setScreen (QScreen* scr);
  };
}
