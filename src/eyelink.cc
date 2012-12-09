#include <iostream>
using namespace std;

#include "qexperiment.h"
using namespace plstim;


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
  if (! check_eyelink (open_eyelink_connection (eyelink_dummy))) {
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
  qDebug () << "get input key";
  KeyInput& key_event = event->key;
  key_event.type = KEYINPUT_EVENT;
  key_event.state = key_up ? ELKEY_DOWN : ELKEY_UP;
  key_up = !key_up;
  key_event.key = ENTER_KEY;
  key_event.modifier = ELKMOD_NONE;
  key_event.unicode = '\n';
  string str;
  cin >> str;
  return 1;
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
  qDebug () << "Setup calibration display";
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
  // No special cleanup in hooks.exit_cal_display_hook
  // Dimension of the camera image
  hooks.setup_image_display_hook = el_setup_image_display;
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

  // Run the calibration
  check_eyelink (do_tracker_setup ());
}
