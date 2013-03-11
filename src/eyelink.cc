#include <iostream>
using namespace std;

#include <QtWidgets>

#include "elcalibration.h"
#include "engine.h"
using namespace plstim;

bool
Engine::check_eyelink (INT16 errcode, const QString & func_name)
{
  if (errcode) {
    emit error (QString ("EyeLink error on %1").arg (func_name),
	    eyelink_get_error (errcode, func_name.toLocal8Bit ().data ()));
    return false;
  }
  return true;
}

void
Engine::load_eyelink ()
{
  qDebug () << "opening EyeLink";
  if (! check_eyelink (open_eyelink_connection (eyelink_dummy),
		       "open_eyelink_connection")) {
    eyelink_connected = false;
    return;
  }
  eyelink_connected = true;

  // Data sent during the experiment
  check_eyelink (eyecmd_printf ("link_sample_data = LEFT,RIGHT,GAZE,GAZERES,AREA,STATUS"), "eyecmd_printf");
  check_eyelink (eyecmd_printf ("link_event_filter = LEFT,RIGHT,FIXATION,SACCADE,BLINK,MESSAGE"), "eyecmd_printf");
}

static INT16 ELCALLBACK
el_setup_image_display (void* userdata, INT16 width, INT16 height)
{
    Q_UNUSED (userdata);
    qDebug () << "EyeLink camera image resolution:" << width << "×" << height;
    return 0;
}

static INT16 ELCALLBACK
el_image_title (void* userdata, char* title)
{
    Q_UNUSED (userdata);
    qDebug () << "EyeLink camera image title:" << title;
    return 0;
}

static INT16 ELCALLBACK
el_draw_image (void* userdata, INT16 width, INT16 height, byte* pixels)
{
  auto xp = static_cast<Engine*> (userdata);
  auto calib = xp->calibrator;
  if (calib != NULL) {
    QImage simg (pixels, width, height, QImage::Format_ARGB32);
    auto rimg = simg.rgbSwapped ();
    auto pixmap = QPixmap::fromImage (rimg);
    if (calib->camera)
      calib->camera->setPixmap (pixmap);
    else
      calib->camera = calib->sc.addPixmap (pixmap);
    // Center the camera image on screen
    calib->camera->setOffset ((calib->sc.width () - width)/2,
			      (calib->sc.height () - height)/2);
  }
  return 0;
}

static INT16 ELCALLBACK
el_clear_cal_display (void* userdata)
{
  auto xp = static_cast<Engine*> (userdata);
  auto calib = xp->calibrator;
  if (calib != NULL)
    calib->clear ();
  return 0;
}

static INT16 ELCALLBACK
el_erase_cal_target (void* userdata)
{
  auto xp = static_cast<Engine*> (userdata);
  auto calib = xp->calibrator;
  calib->erase_target ();
  return 0;
}

static INT16 ELCALLBACK
el_draw_cal_target (void* userdata, float x, float y)
{
  auto xp = static_cast<Engine*> (userdata);
  auto calib = xp->calibrator;
  calib->add_target (x, y);
  return 0;
}

static INT16 ELCALLBACK
el_play_target_beep (void* userdata, EL_CAL_BEEP beep_type)
{
    Q_UNUSED (userdata);
    qDebug () << "beep";
    return 0;
}

static bool key_up = true;

static INT16 ELCALLBACK
el_get_input_key (void* userdata, InputEvent* event)
{
  auto xp = static_cast<Engine*> (userdata);

  // Process any event in the application
  QCoreApplication::instance ()->processEvents ();

  // Check for key events in the queue
  auto calib = xp->calibrator;
  if (! calib->key_queue.isEmpty ()) {
    KeyInput* ki = calib->key_queue.dequeue ();
    memcpy (event, ki, sizeof (KeyInput));
    delete ki;
    return 1;
  }
  
  return 0;
}

static INT16 ELCALLBACK
el_printf_hook (void* userdata, const char* msg)
{
  auto xp = static_cast<Engine*> (userdata);
  xp->error (msg);
  return 0;
}

static INT16 ELCALLBACK
el_setup_display (void* userdata)
{
  qDebug () << "Setup EyeLink calibration display";
  auto xp = static_cast<Engine*> (userdata);

  xp->calibrator = new EyeLinkCalibrator ();
  xp->calibrator->setWindowFlags (Qt::Dialog|Qt::FramelessWindowHint);

  // Display the calibrator in full screen
  xp->calibrator->setScreen (xp->displayScreen ());
  xp->calibrator->show ();

  // Focus the new widget
  xp->calibrator->setFocus (Qt::OtherFocusReason);
  // Remove mouse pointer on the calibrator
  xp->calibrator->setCursor (QCursor (Qt::BlankCursor));
  // Set the correct dimensions for the scene
  xp->calibrator->sc.setSceneRect (0, 0, xp->setup ()->horizontalResolution (), xp->setup ()->verticalResolution ());
  // Set calibrator colours
  auto bg = QColor ("black");//xp->get_colour ("eyelink_background");
  if (! bg.spec () == QColor::Invalid)
    xp->calibrator->sc.setBackgroundBrush (bg);

  return 0;
}

static INT16 ELCALLBACK
el_exit_cal_display (void* userdata)
{
  auto xp = static_cast<Engine*> (userdata);
  if (xp->calibrator) {
    xp->calibrator->hide ();
    delete xp->calibrator;
    xp->calibrator = NULL;
  }
  return 0;
}

static INT16 ELCALLBACK
el_exit_image_display (void* userdata)
{
  auto xp = static_cast<Engine*> (userdata);
  xp->calibrator->remove_camera ();
  return 0;
}

void
Engine::calibrate_eyelink ()
{
  qDebug () << "Calibrating EyeLink";

  // Define calibration hooks
  HOOKFCNS2 hooks;
  memset (&hooks, 0, sizeof (HOOKFCNS2));

  // Version of EyeLink hooks we use
  hooks.major = 1;
  hooks.minor = 0;
  // Register the Engine instance
  hooks.userData = (void*) this;
  // Calibration display setup
  hooks.setup_cal_display_hook = el_setup_display;
  // Cleanup display setup
  hooks.exit_cal_display_hook = el_exit_cal_display;
  // Dimension of the camera image
  hooks.setup_image_display_hook = el_setup_image_display;
  // Dimension of the camera image
  hooks.exit_image_display_hook = el_exit_image_display;
  // Title of the camera image
  hooks.image_title_hook = el_image_title;
  // Camera image
  hooks.draw_image = el_draw_image;
  // No special cleanup in hooks.exit_image_display_hook
  // Cleanup the display
  hooks.clear_cal_display_hook = el_clear_cal_display;
  // Erase the last target
  hooks.erase_cal_target_hook = el_erase_cal_target;
  // Draw a target
  hooks.draw_cal_target_hook = el_draw_cal_target;
  // Emit a beep sound
  hooks.play_target_beep_hook = el_play_target_beep;
  // TODO: input key hook
  hooks.get_input_key_hook = el_get_input_key;
  // Display an alert message
  hooks.alert_printf_hook = el_printf_hook;

  // Register the calibration hooks
  check_eyelink (setup_graphic_hook_functions_V2 (&hooks),
		 "setup_graphic_hook_functions_V2");

  // Run the calibration
  check_eyelink (do_tracker_setup (), "do_tracker_setup");

  // Remove the calibration window
  if (calibrator) {
    calibrator->hide ();
    delete calibrator;
    calibrator = NULL;
  }

  // De-register the calibration hooks
  // since they interfer with EDF file transfers
  memset (&hooks, 0, sizeof (HOOKFCNS2));
  hooks.major = 1;
  check_eyelink (setup_graphic_hook_functions_V2 (&hooks),
		 "setup_graphic_hook_functions_V2");
}


EyeLinkCalibrator::EyeLinkCalibrator (QWidget* parent)
  : QGraphicsView (),
    target (NULL),
    camera (NULL),
    target_size (10)
{
  setScene (&sc);
  setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  setFrameStyle (QFrame::NoFrame);
}

void
EyeLinkCalibrator::add_key_event (QKeyEvent* event, bool pressed)
{
  auto ki = new KeyInput;
  ki->type = KEYINPUT_EVENT;
  ki->state = pressed ? ELKEY_DOWN : ELKEY_UP;
  switch (event->key ()) {
  case Qt::Key_Up:
    ki->key = CURS_UP;
    break;
  case Qt::Key_Down:
    ki->key = CURS_DOWN;
    break;
  case Qt::Key_Left:
    ki->key = CURS_LEFT;
    break;
  case Qt::Key_Right:
    ki->key = CURS_RIGHT;
    break;
  case Qt::Key_Escape:
    ki->key = ESC_KEY;
    break;
  case Qt::Key_Return:
    ki->key = ENTER_KEY;
    break;
  case Qt::Key_PageUp:
    ki->key = PAGE_UP;
    break;
  case Qt::Key_PageDown:
    ki->key = PAGE_DOWN;
    break;
  case Qt::Key_Tab:
    ki->key = '\t';
    break;
  case Qt::Key_F1:
    ki->key = F1_KEY;
    break;
  case Qt::Key_F2:
    ki->key = F2_KEY;
    break;
  case Qt::Key_F3:
    ki->key = F3_KEY;
    break;
  case Qt::Key_F4:
    ki->key = F4_KEY;
    break;
  case Qt::Key_F5:
    ki->key = F5_KEY;
    break;
  case Qt::Key_F6:
    ki->key = F6_KEY;
    break;
  case Qt::Key_F7:
    ki->key = F7_KEY;
    break;
  case Qt::Key_F8:
    ki->key = F8_KEY;
    break;
  case Qt::Key_F9:
    ki->key = F9_KEY;
    break;
  case Qt::Key_F10:
    ki->key = F10_KEY;
    break;
  default:
    ki->key = event->key ();
  }
  // Translate modifiers
  ki->modifier = 0;
  auto mods = event->modifiers ();
  if (mods & Qt::ShiftModifier)
    ki->modifier |= ELKMOD_LSHIFT;
  else if (mods & Qt::ControlModifier)
    ki->modifier |= ELKMOD_LCTRL;
  else if (mods & Qt::AltModifier)
    ki->modifier |= ELKMOD_LALT;
  else if (mods & Qt::MetaModifier)
    ki->modifier |= ELKMOD_LMETA;
  else if (mods & Qt::KeypadModifier)
    ki->modifier |= ELKMOD_NUM;
  else if (mods & Qt::GroupSwitchModifier)
    ki->modifier |= ELKMOD_MODE;
  // Translate the unicode value
  auto txt = event->text ();
  if (txt.size () == 1)
    ki->unicode = txt.at (0).unicode ();
  else
    ki->unicode = 0;

  key_queue.enqueue (ki);
}

void
EyeLinkCalibrator::keyPressEvent (QKeyEvent* event)
{
  add_key_event (event, true);
}

void
EyeLinkCalibrator::keyReleaseEvent (QKeyEvent* event)
{
  add_key_event (event, false);
}

void
EyeLinkCalibrator::add_target (float x, float y)
{
  target = sc.addEllipse (x-target_size/2, y-target_size/2,
			  target_size, target_size,
			  Qt::NoPen, Qt::red);
}

void
EyeLinkCalibrator::erase_target ()
{
  if (target != NULL) {
    sc.removeItem (target);
    target = NULL;
  }
}

void
EyeLinkCalibrator::remove_camera ()
{
  if (camera != NULL) {
    sc.removeItem (camera);
    camera = NULL;
  }
}

void
EyeLinkCalibrator::clear ()
{
  sc.clear ();
}

void
EyeLinkCalibrator::setScreen (QScreen* scr)
{
    QRect geom = scr->geometry ();
    qDebug () << "Showing calibrator at" << geom.x () << geom.y () << "with size" << geom.width () << "x" << geom.height ();
    move (geom.x (), geom.y ());
    resize (geom.width (), geom.height ());
}
