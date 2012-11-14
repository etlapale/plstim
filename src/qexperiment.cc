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

Page::Page (Page::Type t, const QString& page_title)
  : type (t), title (page_title)
{
  qDebug () << "Creating a page with title" << page_title << endl;
  switch (type) {
  case Page::Type::SINGLE:
    wait_for_key = true;
    nframes = 1;
    break;
  case Page::Type::FRAMES:
    wait_for_key = false;
    nframes = 0;
    break;
  }
}

Page::~Page ()
{
  qDebug () << "Page" << title << "deleted";
}

void
Page::make_active ()
{
  emit page_active ();
}

void
Page::emit_key_pressed (QKeyEvent* evt)
{
  emit key_pressed (evt);
}

void
Page::accept_key (int key)
{
  accepted_keys.insert (key);
}

void
QExperiment::add_page (Page* page)
{
  pages.push_back (page);
}

bool
QExperiment::exec ()
{
  cout << "showing windows" << endl;
  win.show ();

  // Load an experiment if given as command line argument
  auto args = app.arguments ();
  if (args.size () == 2)
    load_experiment (args.at (1));


  cout << "exec()" << endl;
  return app.exec () == 0;
}

void
QExperiment::run_trial ()
{
  cout << "Starting trial " << current_trial << endl;

  current_page = -1;
  //show_page (current_page);
  next_page ();
}

void
QExperiment::show_page (int index)
{
  cout << "show page: " << index << endl;
  Page* p = pages[index];

  switch (p->type) {
  case Page::Type::SINGLE:
    glwidget->show_frame (p->title);
    
    // Workaround a bug on initial frame after fullscreen
    // Maybe it’s a race condition?
    // TODO: in any cases, it should be tracked and solved
    if (current_trial == 0 && index == 0)
      glwidget->show_frame (p->title);
    break;
  case Page::Type::FRAMES:
    glwidget->show_frames ();
    break;
  }

  current_page = index;
  p->make_active ();

  // If no keyboard event expected, go to the next page
  if (! p->wait_for_key)
    next_page ();
}

void
QExperiment::next_page ()
{
  // End of trial
  if ((int) (current_page + 1) == (int) (pages.size ())) {
    // Next trial
    if (++current_trial < ntrials) {
      run_trial ();
    }
    // End of session
    else {
      cout << "end of session" << endl;
      glwidget->normal_screen ();
      session_running = false;
    }
  }
  // There is a next page
  else {
    cout << "showing next page" << endl;
    show_page (current_page + 1);
  }
}

void
QExperiment::glwidget_key_press_event (QKeyEvent* evt)
{
  // Keyboard events are only handled in sessions
  if (! session_running)
    return;

  Page* page = pages[current_page];

  // Go to the next page
  if (page->wait_for_key) {
    // Check if the key is accepted
    if (page->accepted_keys.empty ()
	|| page->accepted_keys.find (evt->key ()) != page->accepted_keys.end ()) {
      page->emit_key_pressed (evt);
      cout << endl;
      next_page ();
    }
  }
}

void
QExperiment::gl_resized (int w, int h)
{
  bool ignore_full_screen = true;

  if (waiting_fullscreen) {

    // Confirm that the resize event is a fullscreen event
    if (ignore_full_screen || 
	(w == res_x_edit->text ().toInt ()
	 && h == res_y_edit->text ().toInt ())) {

      waiting_fullscreen = false;
      session_running = true;

      // Run the trials
      /*for (current_trial = 0;
	   current_trial < ntrials;
	   current_trial++)
	if (! run_trial ()) {
	  cout << "could not run a trial!" << endl;
	  break;
	}*/
      run_trial ();
    }

    // Resize event received while expecting fullscreen
    else {
      cerr << "expecting fullscreen event, but got a simple resize"
	   << endl;
    }
  }
}

static QScriptValue
page_constructor (QScriptContext* ctx, QScriptEngine* engine)
{
  cout << "ctor for page called with "
       << ctx->argumentCount() << " arguments" << endl;

  // Invalid arity for Page()
  if (ctx->argumentCount () == 0 || ctx->argumentCount () > 2) {
    cerr << "invalid arity for Page constructor" << endl;
    return engine->undefinedValue ();
  }

  // Make sure we have a page title
  auto tval = ctx->argument (ctx->argumentCount () - 1);
  if (! tval.isString ()) {
    cerr << "expecting a page title as last argument for Page constructor" << endl;
    return engine->undefinedValue ();
  }

  QObject* obj;
  if (ctx->argumentCount () == 1) {
    obj = new Page (Page::Type::SINGLE, tval.toString ());
  }
  else {
    obj = new Page (Page::Type::FRAMES, tval.toString ());
  }

  return engine->newQObject (obj, QScriptEngine::QtOwnership);
}

static QScriptValue
page_adder (QScriptContext* ctx, QScriptEngine* engine, void* param)
{
  if (ctx->argumentCount () != 1) {
    cerr << "invalid use of add_page()" << endl;
    return engine->undefinedValue ();
  }

  auto qexp = static_cast<QExperiment*> (param);
  auto val = ctx->argument (0);
  if (! val.isQObject ()) {
    cerr << "add_page() argument is not a page (neither a QObject)!" << endl;
  }
  else {
    auto page = dynamic_cast<Page*> (val.toQObject ());
    if (page == NULL) {
      cerr << "add_page() argument is not a page!" << endl;
    }
    else {
      qexp->add_page (page);
    }
  }

  return engine->undefinedValue ();
}

Q_DECLARE_METATYPE (QColor)
Q_DECLARE_METATYPE (QColor*)
Q_DECLARE_METATYPE (QPen)
Q_DECLARE_METATYPE (QPen*)
Q_DECLARE_METATYPE (QPainter*)
Q_DECLARE_METATYPE (QPainterPath)
Q_DECLARE_METATYPE (QPainterPath*)

static QScriptValue
pen_ctor (QScriptContext* ctx, QScriptEngine* engine)
{
  cout << "Creating a new QPainterPath" << endl;
  
  if (ctx->argumentCount () == 0)
    return engine->toScriptValue (new QPen ());
  else {
    auto color = qscriptvalue_cast<QColor*> (ctx->argument (0));
    return engine->toScriptValue (new QPen (*color));
  }
}

static QScriptValue
painterpath_ctor (QScriptContext* ctx, QScriptEngine* engine)
{
  cout << "Creating a new QPainterPath" << endl;
  return engine->toScriptValue (new QPainterPath ());
}

static QScriptValue
color_ctor (QScriptContext* ctx, QScriptEngine* engine)
{
  int r = ctx->argument (0).toInt32 ();
  int g = ctx->argument (1).toInt32 ();
  int b = ctx->argument (2).toInt32 ();

  return engine->toScriptValue (QColor (r, g, b));
}

void
QExperiment::script_engine_exception (const QScriptValue& exception)
{
  cerr << "script engine exception!" << endl;

  if (exception.isError ()) {
    qDebug () << exception.property ("message").toString ();
  }
}

bool
QExperiment::load_experiment (const QString& script_path)
{
  // TODO: cleanup any existing experiment
  ntrials = 0;
  if (script_engine != NULL)
    delete script_engine;

  // Create a new QtScript engine
  script_engine = new QScriptEngine ();
  connect (script_engine,
	   SIGNAL (signalHandlerException (const QScriptValue&)),
	   this,
	   SLOT (script_engine_exception (const QScriptValue&)));

  // Register defined prototypes
  script_engine->setDefaultPrototype (qMetaTypeId<QColor*>(),
				      script_engine->newQObject (&color_proto));
  script_engine->setDefaultPrototype (qMetaTypeId<QPainterPath*>(),
				      script_engine->newQObject (&painterpath_proto));
  script_engine->setDefaultPrototype (qMetaTypeId<QPainter*>(),
				      script_engine->newQObject (&painter_proto));

  // Load the experiment from a QtScript
  qDebug () << "Loading experiment from:" << script_path;
  QFile file (script_path);
  file.open (QIODevice::ReadOnly);

  // Register Page constructor
  QScriptValue page_ctor = script_engine->newFunction (page_constructor);
  QScriptValue page_meta = script_engine->newQMetaObject (&Page::staticMetaObject, page_ctor);
  script_engine->globalObject ().setProperty ("Page", page_meta);

  // Register add_page()
  auto page_add_cb = script_engine->newFunction (page_adder, this);
  script_engine->globalObject ().setProperty ("add_page", page_add_cb);

  // Register QColor() constructor
  auto ctor = script_engine->newFunction (color_ctor, script_engine->newQObject (&color_proto));
  script_engine->globalObject ().setProperty ("QColor", ctor);

  // Register QPen() constructor
  ctor = script_engine->newFunction (pen_ctor, script_engine->newQObject (&pen_proto));
  script_engine->globalObject ().setProperty ("QPen", ctor);

  // Register QPainterPath() constructor
  auto path_ctor = script_engine->newFunction (painterpath_ctor, script_engine->newQObject (&painterpath_proto));
  script_engine->globalObject ().setProperty ("QPainterPath", path_ctor);

  // Register the experiment
  auto xp_obj = script_engine->newQObject (this);
  script_engine->globalObject ().setProperty ("xp", xp_obj);

  // Register ‘glwidget’
  auto glw_obj = script_engine->newQObject (glwidget);
  script_engine->globalObject ().setProperty ("glwidget", glw_obj);

  // Load the experiment script
  QScriptValue res = script_engine->evaluate (file.readAll (),
					     script_path);
  if (res.isError ()) {
    qDebug () << res.property ("message").toString ();
  }

  // Check for exceptions
  if (script_engine->hasUncaughtException ()) {
    cout << "uncaught exception!" << endl;
  }

  file.close ();
  QScriptValue ntrials_val = script_engine->globalObject ().property ("ntrials");
  if (ntrials_val.isNumber ()) {
    ntrials = ntrials_val.toInteger ();
  }

  cout << endl
       << "Experiment loaded:" << endl
       << "  " << ntrials << " trials" << endl
       << "  " << pages.size () << " pages" << endl
       << endl;

  // Initialise the experiment
  setup_updated ();

  return true;
}

void
QExperiment::run_session ()
{
  cout << "Run session!" << endl;

  // Save the splitter position
  splitter_state = splitter->saveState ();

  // Notify that we want to run the trials
  waiting_fullscreen = true;

  current_trial = 0;

  // Set the GL scene full screen
  glwidget->full_screen ();
}

bool
QExperiment::clear_screen ()
{
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

void
QExperiment::about_to_quit ()
{
  // Save the GUI state
  QSize sz = win.size ();
  settings->setValue ("gui_width", sz.width ());
  settings->setValue ("gui_height", sz.height ());
  settings->setValue ("splitter", splitter->saveState ());
}

QExperiment::QExperiment (int & argc, char** argv)
  : app (argc, argv),
    win (),
    save_setup (false),
    glwidget_initialised (false),
    bin_dist (0, 1)
{
  if (! plstim::initialise ())
    error ("could not initialise plstim");

  res_msg = NULL;
  match_res_msg = NULL;
  script_engine = NULL;

  waiting_fullscreen = false;
  session_running = false;
  

  // Initialise the random number generator
  twister.seed (time (NULL));
  for (int i = 0; i < 10000; i++)
    (void) bin_dist (twister);


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

  splitter = new QSplitter (&win);
  win.setCentralWidget (splitter);

  // Create an OpenGL widget
  QGLFormat fmt;
  fmt.setDoubleBuffer (true);
  fmt.setVersion (3, 0);
  fmt.setDepth (false);
  fmt.setSwapInterval (1);
  glwidget = NULL;
  set_glformat (fmt);
  //glwidget = new MyGLWidget (fmt, &win);
  cout << "OpenGL version " << glwidget->format ().majorVersion ()
       << '.' << glwidget->format ().minorVersion () << endl;
  if (! glwidget->format ().doubleBuffer ())
    error ("double buffering not supported");

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
    for (auto g : groups) {
      if (g != "General") {
	setup_cbox->addItem (g);
      }
    }

    // TODO: check if there was a ‘last’ setup
    // TODO: make sure there really are existing setups
    
    // Restore setup
    update_setup ();
  }

  // Restore GUI state
  if (settings->contains ("gui_width"))
    win.resize (settings->value ("gui_width").toInt (),
		settings->value ("gui_height").toInt ());
  if (settings->contains ("splitter")) {
    qDebug () << "restoring splitter state from config";
    splitter->restoreState (settings->value ("splitter").toByteArray ());
  }

  // Constrain the screen selector
  screen_sbox->setMinimum (0);
  screen_sbox->setMaximum (dsk.screenCount () - 1);

  save_setup = true;

  // Close event termination
  connect (&app, SIGNAL (aboutToQuit ()),
	   this, SLOT (about_to_quit ()));
}

void
QExperiment::setup_updated ()
{
  cout << "Setup updated" << endl;
  // Make sure converters are up to date
  update_converters ();

  // Notify the experiment that a change occured
  if (script_engine != NULL) {
    auto this_obj = script_engine->newQObject (this);
    // Check for an update_configuration function
    auto val = script_engine->globalObject ().property ("update_configuration");
    // Make sure it’s a function
    if (val.isFunction ()) {
      auto res = val.call (this_obj);
      if (res.isError ()) {
	qDebug () << res.property ("message").toString ();
      }
    }

    // Make sure that every frame is repainted
    // TODO: this can be done in parallel
    if (glwidget_initialised) {
      qDebug () << "repainting frames on a" << tex_size << "pixels texture";
      // Make sure the new size is acceptable on the target screen
      auto geom = dsk.screenGeometry (screen_sbox->value ());
      if (tex_size > geom.width () || tex_size > geom.height ()) {
	// TODO: use the message GUI logging facilities
	qDebug () << "texture too big for the screen";
	return;
      }

      // Image on which the frames are painted
      QImage img (tex_size, tex_size, QImage::Format_RGB32);

      // Check for a global named frame painter function
      auto val = script_engine->globalObject ().property ("paint_frame");

      // Painter
      QPainter painter;
      auto painter_obj = script_engine->newVariant (QVariant::fromValue (&painter));

      for (auto p : pages) {

	// Single frames
	if (p->type == Page::Type::SINGLE && val.isFunction ()) {
	  painter.begin (&img);

	  QScriptValueList args;
	  args << p->title;
	  args << painter_obj;
	  args << 0;
	  
	  auto res = val.call (this_obj, args);
	  if (res.isError ()) {
	    qDebug () << res.property ("message").toString ();
	  }

	  painter.end ();
	  img.save (QString ("page-") + p->title + ".png");
	  glwidget->add_frame (p->title, img);
	}

	// Multiple frames
	if (p->type == Page::Type::FRAMES && val.isFunction ()) {
	  qDebug () << "painting" << p->frameCount () << "frames for" << p->title;

	  glwidget->delete_unamed_frames ();

	  for (int i = 0; i < p->frameCount (); i++) {

	    painter.begin (&img);

	    QScriptValueList args;
	    args << p->title;
	    args << painter_obj;
	    args << i;
	    
	    auto res = val.call (this_obj, args);
	    if (res.isError ()) {
	      qDebug () << res.property ("message").toString ();
	    }

	    painter.end ();
	    QString filename;
	    filename.sprintf ("page-%s-%04d.png", qPrintable (p->title), i);
	    img.save (filename);
	    glwidget->add_frame (img);
	  }
	}
      }

      glwidget->update_texture_size (tex_size, tex_size);
    }
  }
}

void
QExperiment::glwidget_gl_initialised ()
{
  glwidget_initialised = true;
  setup_updated ();
}

void
QExperiment::normal_screen_restored ()
{
  splitter->addWidget (glwidget);
  splitter->restoreState (splitter_state);
}

void
QExperiment::set_glformat (QGLFormat glformat)
{
  qDebug () << "set_glformat ()" << endl;
  splitter_state = splitter->saveState ();

  auto old_widget = glwidget;
  if (old_widget != NULL) {
    //delete old_widget;
    //old_widget->waiting_deletion = true;
    //delete old_widget;
    //old_widget->setParent (NULL);
    //old_widget->hide ();
    delete old_widget;
    //old_widget->deleteLater ();
    /*connect (old_widget, SIGNAL (destroyed ()),
	     this, SLOT (old_glwidget_destroyed ()));*/
  }
  else {
    cout << "old widget is NULL" << endl;
  }

  glwidget = new MyGLWidget (glformat, &win);

  splitter->addWidget (glwidget);

  // Re-add the glwidget to the splitter when fullscreen ends
  connect (glwidget, SIGNAL (normal_screen_restored ()),
	   this, SLOT (normal_screen_restored ()));
  connect (glwidget, SIGNAL (gl_initialised ()),
	   this, SLOT (glwidget_gl_initialised ()));
  connect (glwidget, SIGNAL (gl_resized (int, int)),
	   this, SLOT (gl_resized (int, int)));
  connect (glwidget, SIGNAL (key_press_event (QKeyEvent*)),
	   this, SLOT (glwidget_key_press_event (QKeyEvent*)));

  splitter->addWidget (glwidget);
  splitter->restoreState (splitter_state);

  cout << "new glwidget in place" << endl;
}

void
QExperiment::set_swap_interval (int swap_interval)
{
  cout << "calling set_swap_interval!" << endl;

  auto glformat = glwidget->format ();
  auto current_si = glformat.swapInterval ();
  if (current_si != swap_interval) {
    cout << "updating swap interval" << endl;
    glformat.setSwapInterval (swap_interval);
    set_glformat (glformat);
  }
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

  if (script_engine != NULL)
    delete script_engine;
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
  setup_updated ();
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

  setup_updated ();
}

float
QExperiment::monitor_rate () const
{
  return refresh_edit->text ().toFloat ();
}


int
main (int argc, char* argv[])
{
  QExperiment xp (argc, argv);
  cout << xp.exec () << endl;
  return 0;
}
