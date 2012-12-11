#include <iostream>
using namespace std;

#include "elcalibration.h"
#include "qexperiment.h"
using namespace plstim;

bool
QExperiment::check_eyelink (INT16 errcode, const QString & func_name)
{
  if (errcode) {
    error (QString ("EyeLink error: %1")
	   .arg (eyelink_get_error (errcode,
				    func_name.toLocal8Bit ().data ())));
    return false;
  }
  return true;
}

bool
QExperiment::check_eyelink (INT16 errcode)
{
  switch (errcode) {
  case 0:
    return true;
  default:
    error (QString ("Unknown EyeLink error: %1").arg (errcode));
  }
  return false;
}

void
QExperiment::load_eyelink ()
{
  qDebug () << "opening EyeLink";
  if (! check_eyelink (open_eyelink_connection (eyelink_dummy),
		       "open_eyelink_connection")) {
    eyelink_connected = false;
    return;
  }
  eyelink_connected = true;

  // Data sent during the experiment
  check_eyelink (eyecmd_printf ("link_sample_data = LEFT,RIGHT,GAZE,GAZERES,AREA,STATUS"));
  check_eyelink (eyecmd_printf ("link_event_filter = LEFT,RIGHT,FIXATION,SACCADE,BLINK,MESSAGE"));
}

static INT16 ELCALLBACK
el_setup_image_display (void* userdata, INT16 width, INT16 height)
{
  qDebug () << "EyeLink camera image resolution:" << width << "×" << height;
  return 0;
}

static INT16 ELCALLBACK
el_image_title (void* userdata, char* title)
{
  qDebug () << "EyeLink camera image title:" << title;
  return 0;
}

static INT16 ELCALLBACK
el_draw_image (void* userdata, INT16 width, INT16 height, byte* pixels)
{
  qDebug () << "Receiving camera image of size" << width << "×" << height;
  return 0;
}

static INT16 ELCALLBACK
el_clear_cal_display (void* userdata)
{
  qDebug () << "Clear display";
  return 0;
}

static INT16 ELCALLBACK
el_erase_cal_target (void* userdata)
{
  qDebug () << "Erase calibration target";
  return 0;
}

static INT16 ELCALLBACK
el_draw_cal_target (void* userdata, float x, float y)
{
  qDebug () << "Calibration target at" << x << y;
  return 0;
}

static INT16 ELCALLBACK
el_play_target_beep (void* userdata, EL_CAL_BEEP beep_type)
{
  qDebug () << "beep";
  return 0;
}

static bool key_up = true;

static INT16 ELCALLBACK
el_get_input_key (void* userdata, InputEvent* event)
{
  auto xp = static_cast<QExperiment*> (userdata);

  // Process any event in the application
  xp->app.processEvents ();
  // Check for key events in the queue
  auto calib = xp->calibrator;
  if (! calib->key_queue.isEmpty ()) {
    qDebug () << "found a key event in the queue";
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
  auto xp = static_cast<QExperiment*> (userdata);
  xp->error (msg);
  return 0;
}

static INT16 ELCALLBACK
el_setup_display (void* userdata)
{
  qDebug () << "Setup EyeLink calibration display";
  auto xp = static_cast<QExperiment*> (userdata);

  xp->calibrator = new EyeLinkCalibrator ();
  xp->calibrator->setWindowFlags (Qt::Dialog|Qt::FramelessWindowHint);
  // Put the calibration window at the correct position
  xp->calibrator->show ();
  xp->calibrator->move (xp->off_x_edit->value (), xp->off_y_edit->value ());
  // Display the calibrator in full screen
  xp->calibrator->showFullScreen ();
  // Focus the new widget
  xp->calibrator->setFocus (Qt::OtherFocusReason);
#if 0
  xp->calibrator->setParent (NULL, Qt::Dialog|Qt::FramelessWindowHint);
  xp->calibrator->setCursor (QCursor (Qt::BlankCursor));
#endif

  return 0;
}

static INT16 ELCALLBACK
el_exit_cal_display (void* userdata)
{
  qDebug () << "Exit calibration display";
  auto xp = static_cast<QExperiment*> (userdata);
  delete xp->calibrator;
  return 0;
}

static INT16 ELCALLBACK
el_exit_image_display (void* userdata)
{
  qDebug () << "Exit image display";
  //auto xp = static_cast<QExperiment*> (userdata);
  //delete xp->calibrator;
  return 0;
}

void
QExperiment::calibrate_eyelink ()
{
  qDebug () << "Calibrating EyeLink";

  // Define calibration hooks
  HOOKFCNS2 hooks;
  memset (&hooks, 0, sizeof (HOOKFCNS2));

  // Version of EyeLink hooks we use
  hooks.major = 1;
  hooks.minor = 0;
  // Register the QExperiment instance
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
  check_eyelink (setup_graphic_hook_functions_V2 (&hooks));

#ifdef DUMMY_EYELINK
  auto worker = new CalibrationWorker (&hooks);
  auto thread = new QThread (this);
  connect (thread, SIGNAL (started ()),
	   worker, SLOT (run_calibration ()));
  connect (thread, SIGNAL (finished ()),
	   worker, SLOT (deleteLater ()));
  worker->moveToThread (thread);
  thread->start ();
  thread->wait ();
#else
  // Run the calibration
  check_eyelink (do_tracker_setup ());
#endif
}


EyeLinkCalibrator::EyeLinkCalibrator (QWidget* parent)
  : QGraphicsView ()//parent)
{
  qDebug () << "scene set to" << scene ();
  //setScene (&sc);
  qDebug () << "scene set to" << scene ();
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
  default:
    qDebug () << "unknown key press in EyeLink calibrator" << event->key ();
    ki->key = event->key ();
    //return;
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

  qDebug () << "Adding translated key event to the queue";
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

#ifdef DUMMY_EYELINK
CalibrationWorker::CalibrationWorker (HOOKFCNS2* hooks)
{
  this->hooks = *hooks;
}

void
CalibrationWorker::run_calibration ()
{
  qDebug () << "starting EyeLink calibration thread";

  hooks.setup_cal_display_hook (hooks.userData);
  hooks.exit_cal_display_hook (hooks.userData);

  QThread::currentThread ()->quit ();
}
#endif // DUMMY_EYELINK

