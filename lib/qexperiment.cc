// plstim/qexperiment.cc – Base class for experimental stimuli

//#include <unistd.h>


#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;


#include "qexperiment.h"
using namespace plstim;


#include <QMenuBar>
#include <QHostInfo>
#include <QtDebug>



#if 0
#ifdef HAVE_XRANDR
  int evt_base, err_base;
  int maj, min;
  if (! XRRQueryExtension (dpy, &evt_base, &err_base)
      || ! XRRQueryVersion (dpy, &maj, &min)) {
    cerr << "error: RandR extension missing" << endl;
    return false;
  }
  if (maj < 1 || (maj == 1 && min < 3)) {
    cerr << "error: at least version 1.3 of RandR is required, but "
         << maj << '.' << min << " was found" << endl;
    return false;
  }
  cout << "RandR version " << maj << '.' << min << endl;

  int min_width, min_height, max_width, max_height;
  XRRGetScreenSizeRange (dpy, root, &min_width, &min_height,
			 &max_width, &max_height);
  cout << "Screen sizes between " << min_width << "×" << min_height
       << " and " << max_height << "×" << max_height << endl;

  XRRScreenResources* res;
  if (! (res = XRRGetScreenResources (dpy, root))) {
    cerr << "error: could not get screen resources" << endl;
    return false;
  }

  // Use the default output if none specified
  RROutput rro;
  XRROutputInfo* out_info;
  if (routput.empty ()) {
    // Use the primary output by default
    rro = XRRGetOutputPrimary (dpy, root);
    if (rro) {
      cout << "Using primary output: " << (long) rro << endl;
    }
    // If no primary output is defined check for the first
    // enabled output
    else {
      rro = res->outputs[0];
      cout << "Using first output: " << (long) rro << endl;
    }
    // Fetch the selected output information
    out_info = XRRGetOutputInfo (dpy, res, rro);
  }
  else {
    bool found = false;
    // Search for a matching output name
    for (int i = 0; i < res->noutput; i++) {
      rro = res->outputs[i];
      out_info = XRRGetOutputInfo (dpy, res, rro);
      if (strncmp (routput.c_str (),
		   out_info->name, out_info->nameLen) == 0) {
	found = true;
	break;
      }
      XRRFreeOutputInfo (out_info);
    }
    // No output with given name
    if (! found) {
      cerr << "error: could not find specified output: "
           << routput << endl;
      return false;
    }
  }

  // Make sure there is a monitor attached
  if (! out_info->crtc) {
    cerr << "error: no monitor attached to output " << routput << endl;
    return false;
  }

  // Get the attached monitor information
  auto crtc_info = XRRGetCrtcInfo (dpy, res, out_info->crtc);

  // Search the current monitor mode
  bool found = false;
  float mon_rate;
  for (int i = 0; i < res->nmode; i++) {
    auto mode_info = res->modes[i];
    if (mode_info.id == crtc_info->mode) {
      if (mode_info.hTotal && mode_info.vTotal) {
	mon_rate = ((float) mode_info.dotClock)
	  / ((float) mode_info.hTotal * (float) mode_info.vTotal);
	found = true;
      }
      break;
    }
  }
  // Monitor refresh rate not found
  if (! found) {
    cerr << "error: refresh rate for output " << routput << " not found" << endl;
    return false;
  }

  cout << "Selected CRTC:" << endl
       << "  Refresh rate: " << mon_rate << " Hz" << endl
       << "  Offset: " << crtc_info->x << " " << crtc_info->y << endl
       << "  Size: " << crtc_info->width << "×" << crtc_info->height << endl;

  // Store the refresh rate in the setup
  if (setup->refresh != 0 && setup->refresh != mon_rate) {
    cerr << "error: refresh rate in setup file ("
         << setup->refresh << " Hz) doest not match actual refresh rate (" << mon_rate << " Hz)" << endl;
    return false;
  }
  setup->refresh = mon_rate;

  // Place the window on the monitor
  int win_x = crtc_info->x;
  int win_y = crtc_info->y;
  
  // Compute the required number of frames in a trial
  int coef = (int) nearbyintf (mon_rate / wanted_frequency);
  if ((mon_rate/coef - wanted_frequency)/wanted_frequency > 0.01) {
    cerr << "error: cannot set monitor frequency at 1% of the desired frequency" << endl;
    return false;
  }

  nframes = (int) nearbyintf ((mon_rate/coef)*(dur_ms/1000.));
  swap_interval = coef;

  // Wait for changes to be applied
  XSync (dpy, False);

  // Cleanup
  XRRFreeCrtcInfo (crtc_info);
  XRRFreeOutputInfo (out_info);
  XRRFreeScreenResources (res);
  
#else	// ! HAVE_XRANDR
  int win_x = 0, win_y = 0;
  // Compute the required number of frames in a trial
  int coef = (int) nearbyintf (setup->refresh / wanted_frequency);
  if ((setup->refresh/coef - wanted_frequency)/wanted_frequency > 0.01) {
    cerr << "error: cannot set monitor frequency at 1% of the desired frequency" << endl;
    return false;
  }

  nframes = (int) nearbyintf ((setup->refresh/coef)*(dur_ms/1000.));
  swap_interval = coef;
#endif	// HAVE_XRANDR
  cout << "Setting the number of frames to " << nframes
       << " (frame changes every " << coef << " refresh)" << endl;
}
#endif

bool
QExperiment::exec ()
{
  win.show ();
  return app.exec () == 0;
}

bool
QExperiment::run_session ()
{
  cout << "Run session!" << endl;

  // Create a fullscreen window
  //auto dlg = new QDialog (&win);
  /*auto lay = new QHBoxLayout (dlg);
  lay->setContentsMargins (0, 0, 0, 0);
  lay->addWidget (glwidget);
  dlg->setLayout (lay);*/
  //dlg->showFullScreen ();
  //int res = dlg->exec ();
  //dlg->show ();
  //
  glwidget->full_screen ();

  for (current_trial = 0; current_trial < ntrials; current_trial++)
    if (! run_trial ())
      return false;
  return true;
}

bool
QExperiment::wait_any_key ()
{
#if 0
  bool pressed = false;
  XEvent evt;
  while (! pressed) {
    XNextEvent ((Display*) nat_dpy, &evt);
    if (evt.type == KeyPress)
      pressed = true;
  }
  return true;
#endif
  cerr << "NYI" << endl;
  return false;
}

#if 0
bool
QExperiment::wait_for_key (const std::vector<KeySym>& accepted_keys,
			  KeySym* pressed_key)
{
#if 0
  KeySym keysym;
  XComposeStatus compose;
  const int buflen = 10;
  char buf[buflen];
  XEvent evt;
  for (;;) {
    XNextEvent ((Display*) nat_dpy, &evt);
    if (evt.type == KeyPress) {
       XLookupString (&evt.xkey, buf, buflen, &keysym, &compose);
      for (auto k : accepted_keys)
	if (k == keysym) {
	  *pressed_key = keysym;
	  return true;
      }
    }
  }
#endif
  cerr << "NYI" << endl;
  return false;
}
#endif

bool
QExperiment::add_frame (const std::string& name, const QImage& img)
{
  return glwidget->add_frame (name, img);
}

bool
QExperiment::show_frame (const std::string& frame_name)
{
#if 0
  // Set the current frame
  glBindTexture (GL_TEXTURE_2D, special_frames[frame_name]);
  glUniform1i (texloc, 0);

  // Draw the frame with triangles
  glDrawArrays (GL_TRIANGLES, 0, 6);
  
  // Swap
  eglSwapBuffers (egl_dpy, sur);

  return true;
#endif
  cerr << "NYI" << endl;
  return false;
}

bool
QExperiment::show_frames ()
{
  for (int i = 0; i < nframes; i++) {
#if 0
#if BUGGY_EGL_SWAP_INTERVAL
    // Simulate an eglSwapInterval()
    for (int j = 0; j < swap_interval; j++) {
#else
    // Set the swap interval
    if (swap_interval != 1)
      //cout << "swap: " << swap_interval << endl;
      //EGLint si = 2;
      eglSwapInterval (egl_dpy, swap_interval);
#endif
      glClear (GL_COLOR_BUFFER_BIT);

      // Set the current frame
      glBindTexture (GL_TEXTURE_2D, tframes[i]);
      glUniform1i (texloc, 0);

      // Draw the frame with triangles
      glDrawArrays (GL_TRIANGLES, 0, 6);

      // Swap
      eglSwapBuffers (egl_dpy, sur);
      //sleep (2);
#if BUGGY_EGL_SWAP_INTERVAL
    }
#endif
#endif
  }

  cerr << "NYI" << endl;
  return false;
  //return true;
}

bool
QExperiment::clear_screen ()
{
#if 0
  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers (egl_dpy, sur);

  return true;
#endif
  cerr << "NYI" << endl;
  return false;
}

void
QExperiment::error (const std::string& msg)
{
  cerr << "error: " << msg << endl;
  app.exit (1);
}

void
QExperiment::quit ()
{
  cout << "Goodbye!" << endl;
  app.quit ();
}

QExperiment::QExperiment (int & argc, char** argv)
  : app (argc, argv),
    win (),
    swap_interval (1),
    save_setup (false),
    glwidget_initialised (false)
{
  if (! plstim::initialise ())
    error ("could not initialise plstim");

  res_msg = NULL;
  match_res_msg = NULL;

  // Get the experimental setup

  // Check for OpenGL
  if (! QGLFormat::hasOpenGL ())
    error ("OpenGL not supported");
  auto flags = QGLFormat::openGLVersionFlags ();
  if (flags & QGLFormat::OpenGL_Version_4_0)
    cout << "OpenGL 4.0 is supported" << endl;
  else if (flags & QGLFormat::OpenGL_Version_3_0)
    cout << "OpenGL 3.0 is supported" << endl;
  else
    error ("OpenGL >=3.0 required");

  // Create an OpenGL widget
  QGLFormat fmt;
  fmt.setDoubleBuffer (true);
  fmt.setVersion (3, 0);
  fmt.setDepth (false);
  fmt.setSwapInterval (1);
  glwidget = new MyGLWidget (fmt, &win);
  splitter = new QSplitter (&win);
  win.setCentralWidget (splitter);
  cout << "OpenGL version " << glwidget->format ().majorVersion ()
       << '.' << glwidget->format ().minorVersion () << endl;
  if (! glwidget->format ().doubleBuffer ())
    error ("double buffering not supported");
  if (glwidget->format ().swapInterval () != swap_interval) {
    error ("could not set the swap interval");
  }

  // Create a basic menu
  auto menu = win.menuBar ()->addMenu (QMenu::tr ("&Experiment"));
  // Run the simulation in full screen
  auto action = menu->addAction ("&Run");
  action->setShortcut(tr("Ctrl+R"));
  action->setStatusTip(tr("&Run the simulation"));
  connect (action, SIGNAL (triggered ()), this, SLOT (run_session ()));
  // Terminate the program
  action = menu->addAction ("&Quit");
  action->setShortcut(tr("Ctrl+Q"));
  action->setStatusTip(tr("&Quit the program"));
  connect (action, SIGNAL (triggered ()), this, SLOT (quit ()));

  // Top toolbar
  //toolbar = new QToolBar ("Simulation");
  //action = 

  // Left toolbox
  auto tbox = new QToolBox;
  splitter->addWidget (tbox);
  splitter->addWidget (glwidget);

  // Re-add the glwidget to the splitter when fullscreen ends
  connect (glwidget, SIGNAL (normal_screen ()),
	   this, SLOT (normal_screen));

  // Experimental setup
  auto setup_widget = new QWidget;
  setup_cbox = new QComboBox;
  screen_sbox = new QSpinBox;
  connect (screen_sbox, SIGNAL (valueChanged (int)),
	   this, SLOT (setup_param_changed ()));
  res_x_edit = new QLineEdit;
  auto res_valid = new QIntValidator (1, 16384);
  res_x_edit->setValidator (res_valid);
  res_x_edit->setMaxLength (5);
  connect (res_x_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  res_y_edit = new QLineEdit;
  res_y_edit->setValidator (res_valid);
  res_y_edit->setMaxLength (5);
  connect (res_y_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  phy_width_edit = new QLineEdit;
  phy_width_edit->setValidator (res_valid);
  phy_width_edit->setMaxLength (5);
  connect (phy_width_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  phy_height_edit = new QLineEdit;
  phy_height_edit->setValidator (res_valid);
  phy_height_edit->setMaxLength (5);
  connect (phy_height_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  dst_edit = new QLineEdit;
  dst_edit->setValidator (res_valid);
  dst_edit->setMaxLength (5);
  connect (dst_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  lum_min_edit = new QLineEdit;
  auto lum_valid = new QDoubleValidator (0, 16384, 2);
  lum_min_edit->setValidator (lum_valid);
  lum_min_edit->setMaxLength (5);
  connect (lum_min_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  lum_max_edit = new QLineEdit;
  lum_max_edit->setValidator (lum_valid);
  lum_max_edit->setMaxLength (5);
  connect (lum_max_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));
  refresh_edit = new QLineEdit;
  refresh_edit->setValidator (res_valid);
  refresh_edit->setMaxLength (5);
  connect (refresh_edit, SIGNAL (textChanged (const QString&)),
	   this, SLOT (setup_param_changed ()));

  auto flayout = new QFormLayout;
  flayout->addRow ("Setup name", setup_cbox);
  flayout->addRow ("Screen", screen_sbox);
  flayout->addRow ("Horizontal resolution", res_x_edit);
  flayout->addRow ("Vertical resolution", res_y_edit);
  flayout->addRow ("Physical width", phy_width_edit);
  flayout->addRow ("Physical height", phy_height_edit);
  flayout->addRow ("Distance", dst_edit);
  flayout->addRow ("Minimum luminance", lum_min_edit);
  flayout->addRow ("Maximum luminance", lum_max_edit);
  flayout->addRow ("Refresh rate", refresh_edit);
  setup_widget->setLayout (flayout);
  tbox->addItem (setup_widget, "Setup");

  // Try to fetch back setup
  settings = new QSettings;
  auto groups = settings->childGroups ();

  // No setup previously defined, infer a new one
  if (groups.empty ()) {
    auto hostname = QHostInfo::localHostName ();
    qDebug () << "creating a new setup for" << hostname;
    setup_cbox->addItem (hostname);

    // Search for a secondary screen
    int i;
    for (i = 0; i < dsk.screenCount (); i++)
      if (i != dsk.primaryScreen ())
	break;

    // Get screen geometry
    auto geom = dsk.screenGeometry (i);
    qDebug () << "screen geometry: " << geom.width () << "x"
              << geom.height () << "+" << geom.x () << "+" << geom.y ();
    screen_sbox->setValue (i);
    res_x_edit->setText (QString::number (geom.width ()));
    res_y_edit->setText (QString::number (geom.height ()));
  }

  // Use an existing setup
  else {
    // Add all the setups to the combo box
    for (auto g : groups)
      setup_cbox->addItem (g);

    // TODO: check if there was a ‘last’ setup
    
    // Restore setup
    update_setup ();
  }

  // Constrain the screen selector
  screen_sbox->setMinimum (0);
  screen_sbox->setMaximum (dsk.screenCount () - 1);

  save_setup = true;

  connect (this, SIGNAL (setup_updated ()),
	   this, SLOT (update_converters ()));
  connect (glwidget, SIGNAL (gl_initialised ()),
	   this, SLOT (glwidget_gl_initialised ()));
}

void
QExperiment::glwidget_gl_initialised ()
{
  glwidget_initialised = true;
  emit setup_updated ();
}

void
QExperiment::normal_screen ()
{
  splitter->addWidget (glwidget);
}

Message::Message (Type t, const QString& str)
  : type(t), msg (str)
{
}

void
QExperiment::update_converters ()
{
  distance = dst_edit->text ().toFloat ();

  float hres = res_x_edit->text ().toFloat ()
    / phy_width_edit->text ().toFloat ();
  float vres = res_y_edit->text ().toFloat ()
    / phy_height_edit->text ().toFloat ();
  float err = fabsf ((hres-vres) / (hres+vres));

  // Create an error message if necessary
  if (res_msg == NULL) {
    res_msg = NULL;
    auto label = "too much difference between "
      "horizontal and vertical resolutions";
    if (err > 0.1)
      res_msg = new Message (Message::Type::ERROR, label);
    else if (err > 0.01)
      res_msg = new Message (Message::Type::WARNING, label);
    if (res_msg) {
      res_msg->widgets << res_x_edit << phy_width_edit
		       << res_y_edit << phy_height_edit;
      add_message (res_msg);
    }
  }

  // Remove or change message if possible
  else {
    // Remove message
    if (err <= 0.01) {
      remove_message (res_msg);
      res_msg = NULL;
    }
    // Lower message importance
    else if (res_msg->type == Message::Type::ERROR && err <= 0.1) {
      remove_message (res_msg);
      res_msg->type = Message::Type::WARNING;
      add_message (res_msg);
    }
    // Raise message importance
    else if (res_msg->type == Message::Type::WARNING && err > 0.01) {
      remove_message (res_msg);
      res_msg->type = Message::Type::ERROR;
      add_message (res_msg);
    }
  }

  px_mm = (hres+vres)/2.0;


  // TODO: put that somewhere else
  // Check that configured resolution match current one
  auto geom = dsk.screenGeometry (screen_sbox->value ());
  bool matching_size = geom.width () == res_x_edit->text ().toInt ()
      && geom.height () == res_y_edit->text ().toInt ();

  if (match_res_msg == NULL && ! matching_size) {
    match_res_msg = new Message (Message::Type::ERROR,
       "configured resolution does not match current one");
    // TODO: modify current msg on continuing error
    if (geom.width () != res_x_edit->text ().toInt ())
      match_res_msg->widgets << screen_sbox << res_x_edit;
    else
      match_res_msg->widgets << screen_sbox << res_y_edit;
    add_message (match_res_msg);
  }
  else if (match_res_msg != NULL && matching_size) {
    remove_message (match_res_msg);
    match_res_msg = NULL;
  }
}

void
QExperiment::add_message (Message* msg)
{
  // Store the message
  messages << msg;

  // Mark associated widgets
  for (auto w : msg->widgets) {
    auto p = w->palette ();
    if (msg->type == Message::Type::WARNING)
      p.setColor (QPalette::Base, QColor (0xfe, 0xc9, 0x7d));
    else
      p.setColor (QPalette::Base, QColor (0xfe, 0xab, 0xa0));
    w->setPalette (p);
  }

  qDebug ()
    << (msg->type == Message::Type::ERROR ? "error:" : "warning:" )
    << msg->msg;
}

void
QExperiment::remove_message (Message* msg)
{
  messages.removeOne (msg);

  // TODO: restore to the previous colour
  // TODO: make sure no other message changed the colour
  for (auto w : msg->widgets) {
    auto p = w->palette ();
    p.setColor (QPalette::Base, QColor (0xff, 0xff, 0xff));
    w->setPalette (p);
  }
}

QExperiment::~QExperiment ()
{
  delete settings;

  delete setup_cbox;
  delete res_x_edit;
  delete res_y_edit;
  delete phy_width_edit;
  delete phy_height_edit;
  delete dst_edit;
  delete lum_min_edit;
  delete lum_max_edit;
  delete refresh_edit;

  delete glwidget;
#if 0
  egl_cleanup ();
#endif
}

void
QExperiment::setup_param_changed ()
{
  if (! save_setup)
    return;

  qDebug () << "changing setup param value";

  settings->beginGroup (setup_cbox->currentText ());
  settings->setValue ("scr", screen_sbox->value ());
  settings->setValue ("res_x", res_x_edit->text ());
  settings->setValue ("res_y", res_y_edit->text ());
  settings->setValue ("phy_w", phy_width_edit->text ());
  settings->setValue ("phy_h", phy_height_edit->text ());
  settings->setValue ("dst", dst_edit->text ());
  settings->setValue ("lmin", lum_min_edit->text ());
  settings->setValue ("lmax", lum_max_edit->text ());
  settings->setValue ("rate", refresh_edit->text ());
  settings->endGroup ();

  // Emit the signal
  emit setup_updated ();
}

void
QExperiment::update_setup ()
{
  auto sname = setup_cbox->currentText ();
  qDebug () << "restoring setup for" << sname;

  settings->beginGroup (sname);
  screen_sbox->setValue (settings->value ("scr").toInt ());
  res_x_edit->setText (settings->value ("res_x").toString ());
  res_y_edit->setText (settings->value ("res_y").toString ());
  phy_width_edit->setText (settings->value ("phy_w").toString ());
  phy_height_edit->setText (settings->value ("phy_h").toString ());
  dst_edit->setText (settings->value ("dst").toString ());
  lum_min_edit->setText (settings->value ("lmin").toString ());
  lum_max_edit->setText (settings->value ("lmax").toString ());
  refresh_edit->setText (settings->value ("rate").toString ());
  settings->endGroup ();

  emit setup_updated ();
}
