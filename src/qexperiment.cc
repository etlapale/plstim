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



void*
operator new (std::size_t size, lua_State* lstate, const char* metaname)
{
  void* obj = lua_newuserdata (lstate, size);
  luaL_getmetatable (lstate, metaname);
  lua_setmetatable (lstate, -2);
  return obj;
}

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
  : type (t), title (page_title),
    duration (0)
{
  qDebug () << "Creating a page with title" << page_title << endl;
  switch (type) {
  case Page::Type::SINGLE:
    wait_for_key = true;
    nframes = 1;
    paint_time = PaintTime::EXPERIMENT;
    break;
  case Page::Type::FRAMES:
    wait_for_key = false;
    nframes = 0;
    paint_time = PaintTime::TRIAL;
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
  if (args.size () == 2) {
    load_experiment (args.at (1));
  }


  cout << "exec()" << endl;
  return app.exec () == 0;
}

void
QExperiment::paint_page (Page* page,
			 QImage& img,
			 QPainter* painter)
{
  QElapsedTimer timer;
  //cout << "timer is monotonic: " << timer.isMonotonic () << endl;

  switch (page->type) {
  // Single frames
  case Page::Type::SINGLE:
    painter->begin (&img);

    lua_getglobal (lstate, "paint_frame");
    lua_pushstring (lstate, page->title.toLocal8Bit ().data ());

    // Fetch the QPainter from the registry
    lua_pushlightuserdata (lstate, (void*) painter);
    lua_gettable (lstate, LUA_REGISTRYINDEX);

    lua_pushnumber (lstate, 0);

    if (lua_pcall (lstate, 3, 0, 0) != 0)
      qDebug () << "error: could not call paint_frame ():"
	<< lua_tostring (lstate, -1);

    painter->end ();

    img.save (QString ("page-") + page->title + ".png");
    glwidget->add_frame (page->title, img);
    break;

  // Multiple frames
  case Page::Type::FRAMES:
    qDebug () << "painting" << page->frameCount () << "frames for" << page->title;

    //timer.start ();
    glwidget->delete_unamed_frames ();
    //qDebug () << "deleting unamed took: " << timer.elapsed () << " milliseconds" << endl;
    timer.start ();

    for (int i = 0; i < page->frameCount (); i++) {

      painter->begin (&img);

      lua_getglobal (lstate, "paint_frame");
      lua_pushstring (lstate, page->title.toLocal8Bit ().data ());

      // Fetch the QPainter from the registry
      lua_pushlightuserdata (lstate, (void*) painter);
      lua_gettable (lstate, LUA_REGISTRYINDEX);

      lua_pushnumber (lstate, i);

      if (lua_pcall (lstate, 3, 0, 0) != 0)
	qDebug () << "error: could not call paint_frame ():"
	  << lua_tostring (lstate, -1);

      painter->end ();
      QString filename;
      filename.sprintf ("page-%s-%04d.png", qPrintable (page->title), i);
      //img.save (filename);
      glwidget->add_frame (img);
    }
    qDebug () << "generating frames took: " << timer.elapsed () << " milliseconds" << endl;
    break;
  }
}

void
QExperiment::run_trial ()
{
  cout << "Starting trial " << current_trial << endl;

  // Call the update_configuration () callback
  lua_getglobal (lstate, "new_trial");
  if (lua_isfunction (lstate, -1)) {
    if (lua_pcall (lstate, 0, 0, 0) != 0) {
      qDebug () << "error: could not call update_configuration:"
		<< lua_tostring (lstate, -1);
    }
  }
  lua_pop (lstate, 1);
  // Re-generate per-trial frames
#if 0
  auto paint_func = script_engine->globalObject ().property ("paint_frame");
  if (paint_func.isFunction ()) {
    QImage img (tex_size, tex_size, QImage::Format_RGB32);
    QPainter painter;
    auto painter_obj = script_engine->newVariant (QVariant::fromValue (&painter));

    for (auto p : pages) {
      if (p->paint_time == Page::PaintTime::TRIAL)
	paint_page (p, img, painter,
		    painter_obj, paint_func, this_obj);
    }

  }
#endif

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
      run_trial ();
    }

    // Resize event received while expecting fullscreen
    else {
      cerr << "expecting fullscreen event, but got a simple resize"
	   << endl;
    }
  }
}


static const char* PLSTIM_EXPERIMENT = "plstim::experiment";


static int
l_add_page (lua_State* lstate)
{
  // Page information
  auto page_name = luaL_checkstring (lstate, 1);
  auto page_type = Page::Type::SINGLE;
  float duration = 0;

  // Animated frames
  if (lua_gettop (lstate) > 1) {
    page_type = Page::Type::FRAMES;
    duration = luaL_checknumber (lstate, 2);
    lua_pop (lstate, 1);
  }
  
  // Search for the experiment
  lua_pushstring (lstate, PLSTIM_EXPERIMENT);
  lua_gettable (lstate, LUA_REGISTRYINDEX);
  auto xp = (QExperiment*) lua_touserdata (lstate, -1);
  lua_pop (lstate, 1);

  // Create a new page and add it to the experiment
  auto page = new Page (page_type, page_name);
  page->duration = duration;
  xp->add_page (page);

  // Return the created page

  return 0;
}

static int
l_bin_random (lua_State* lstate)
{
  // Search for the experiment
  lua_pushstring (lstate, PLSTIM_EXPERIMENT);
  lua_gettable (lstate, LUA_REGISTRYINDEX);
  auto xp = (QExperiment*) lua_touserdata (lstate, -1);
  lua_pop (lstate, 1);

  // Generate the random number
  int num = xp->bin_dist (xp->twister);
  lua_pushboolean (lstate, num);
  
  return 1;
}

static int
l_deg2pix (lua_State* lstate)
{
  // Search for the experiment
  lua_pushstring (lstate, PLSTIM_EXPERIMENT);
  lua_gettable (lstate, LUA_REGISTRYINDEX);
  auto xp = (QExperiment*) lua_touserdata (lstate, -1);
  lua_pop (lstate, 1);

  // Distance argument [deg]
  auto degs = luaL_checknumber (lstate, -1);
  lua_pop (lstate, 1);

  // Generate the random number
  auto pixs = xp->deg2pix (degs);
  lua_pushnumber (lstate, pixs);
  
  return 1;
}

static QColor
get_colour (lua_State* lstate, int index)
{
  QColor col;

  if (! lua_istable (lstate, index)) {
    qDebug () << "error: expecting RGB color as last argument";
    return col;
  }

  lua_pushinteger (lstate, 1);
  lua_gettable (lstate, index);
  col.setRedF (luaL_checknumber (lstate, -1));
  lua_pushinteger (lstate, 2);
  lua_gettable (lstate, index);
  col.setGreenF (luaL_checknumber (lstate, -1));
  lua_pushinteger (lstate, 3);
  lua_gettable (lstate, index);
  col.setBlueF (luaL_checknumber (lstate, -1));

  return col;
}

static int
l_qpainter_fill_rect (lua_State* lstate)
{
  qDebug () << "qpainter:fill_rect ()";

  QPainter* painter = (QPainter*) lua_touserdata (lstate, 1);
  int x = luaL_checkint (lstate, 2);
  int y = luaL_checkint (lstate, 3);
  int width = luaL_checkint (lstate, 4);
  int height = luaL_checkint (lstate, 5);

  // Fetch the colour given as argument
  QColor col = get_colour (lstate, 6);

  // TODO: pop the arguments?

  painter->fillRect (x, y, width, height, col);

  return 0;
}

static int
l_qpainter_set_brush (lua_State* lstate)
{
  qDebug () << "qpainter:set_brush ()";

  QPainter* painter = (QPainter*) lua_touserdata (lstate, 1);
  QColor col = get_colour (lstate, 2);

  painter->setBrush (col);

  return 0;
}


static int
l_qpainter_gc (lua_State* lstate)
{
  qDebug () << "[Lua] Garbage collecting a QPainter";

  // Call the C++ destructor
  auto painter = static_cast<QPainter*> (lua_touserdata (lstate, 1));
  painter->~QPainter ();

  return 0;
}

static const struct luaL_reg qpainter_lib_f [] = {
  //{"fill_rect", l_qpainter_fill_rect},
  {NULL, NULL}
};

static const struct luaL_reg qpainter_lib_m [] = {
  {"fill_rect", l_qpainter_fill_rect},
  {"set_brush", l_qpainter_set_brush},
  {"__gc", l_qpainter_gc},
  {NULL, NULL}
};

static int
l_qpainterpath_new (lua_State* lstate)
{
  // Create a new QPainterPath
  auto path = new QPainterPath ();
  lua_pushlightuserdata (lstate, path);

  // Set the matching metatable
  luaL_getmetatable (lstate, "plstim.qpainterpath");
  lua_setmetatable (lstate, -2);

  return 1;
}

static int
l_qpainterpath_add_rect (lua_State* lstate)
{
  // Make sure we got a QPainterPath
  auto path = (QPainterPath*) luaL_checkudata (lstate, 1, "plstim.qpainterpath");

  int x = luaL_checkint (lstate, 2);
  int y = luaL_checkint (lstate, 3);
  int width = luaL_checkint (lstate, 4);
  int height = luaL_checkint (lstate, 5);

  // TODO: pop the arguments?

  path->addRect (x, y, width, height);

  return 0;
}
static int
l_qpainterpath_add_ellipse (lua_State* lstate)
{
  // Make sure we got a QPainterPath
  auto path = (QPainterPath*) luaL_checkudata (lstate, 1, "plstim.qpainterpath");

  int x = luaL_checkint (lstate, 2);
  int y = luaL_checkint (lstate, 3);
  int width = luaL_checkint (lstate, 4);
  int height = luaL_checkint (lstate, 5);

  // TODO: pop the arguments?

  path->addEllipse (x, y, width, height);

  return 0;
}

static const struct luaL_reg qpainterpath_lib_f [] = {
  {"new", l_qpainterpath_new},
  {NULL, NULL}
};

static const struct luaL_reg qpainterpath_lib_m [] = {
  {"add_ellipse", l_qpainterpath_add_ellipse},
  {"add_rect", l_qpainterpath_add_rect},
  {NULL, NULL}
};


static void
register_class (lua_State* lstate,
		const char* fullname, const luaL_reg* methods)
{
  luaL_newmetatable (lstate, fullname);

  // Register the methods
  lua_pushstring (lstate, "__index");
  lua_pushvalue (lstate, -2);
  lua_settable (lstate, -3);
  luaL_openlib (lstate, NULL, methods, 0);
}

int
luaopen_plstim (lua_State* lstate)
{
  register_class (lstate, "plstim.qpainter", qpainter_lib_m);
  luaL_openlib (lstate, "qpainter", qpainter_lib_f, 0);

  register_class (lstate, "plstim.qpainterpath", qpainterpath_lib_m);
  luaL_openlib (lstate, "qpainterpath", qpainterpath_lib_f, 0);
}

bool
QExperiment::load_experiment (const QString& script_path)
{
  // TODO: cleanup any existing experiment
  ntrials = 0;
  if (lstate != NULL)
    lua_close (lstate);

  // Create a new Lua state
  lstate = lua_open ();
  luaL_openlibs (lstate);

  // Register the current experiment
  lua_pushstring (lstate, PLSTIM_EXPERIMENT);
  lua_pushlightuserdata (lstate, this);
  lua_settable (lstate, LUA_REGISTRYINDEX);

  // Create a (TODO: read-only) experiment information table
  lua_newtable (lstate);
  lua_pushstring (lstate, "path");
  lua_pushstring (lstate, script_path.toLocal8Bit ().data ());
  lua_settable (lstate, -3);
  lua_setglobal (lstate, "xp");

  // Register wrappers
  luaopen_plstim (lstate);

  // Register add_page ()
  lua_pushcfunction (lstate, l_add_page);
  lua_setglobal (lstate, "add_page");

  // Register bin_random ()
  lua_pushcfunction (lstate, l_bin_random);
  lua_setglobal (lstate, "bin_random");
  // Register deg2pix ()
  lua_pushcfunction (lstate, l_deg2pix);
  lua_setglobal (lstate, "deg2pix");

  if (luaL_loadfile (lstate, script_path.toLocal8Bit ().data ())
      || lua_pcall (lstate, 0, 0, 0)) {
    qDebug () << "could not load the experiment:"
              << lua_tostring (lstate, -1);
  }

  lua_getglobal (lstate, "ntrials");
  if (lua_isnumber (lstate, -1)) {
    ntrials = (int) lua_tonumber (lstate, -1);
  }
  lua_pop (lstate, 1);

  qDebug () << endl
            << "Experiment loaded:" << endl
	    << " " << ntrials << "trials" << endl
	    << " " << pages.size () << "pages"
	    << endl;

  // Initialise the experiment
  setup_updated ();

  return true;
}

void
QExperiment::run_session ()
{
  // Make sure an experiment is loaded
  if (lstate == NULL)
    return;

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

  lstate = NULL;

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

  if (lstate != NULL) {
    // Compute the best swap interval
    lua_getglobal (lstate, "refresh");
    if (lua_isnumber (lstate, -1)) {
      double wanted_frequency = lua_tonumber (lstate, -1);
      double mon_rate = monitor_rate ();
      double coef = round (mon_rate / wanted_frequency);
      if ((mon_rate/coef - wanted_frequency)/wanted_frequency > 0.01)
	qDebug () << "error: cannot set monitor frequency to 1% of desired frequency";
      qDebug () << "Swap interval:" << coef;
      set_swap_interval (coef);
    }
    lua_pop (lstate, 1);

    // Compute the minimal texture size
    lua_getglobal (lstate, "size");
    if (lua_isnumber (lstate, -1)) {
      double size_degs = lua_tonumber (lstate, -1);
      double size_px = ceil (deg2pix (size_degs));
      qDebug () << "Stimulus size:" << size_px;

      // Minimal base-2 texture size
      tex_size = 1 << (int) floor (log2 (size_px));
      if (tex_size < size_px)
	tex_size <<= 1;

      // Update Lua’s (TODO: read-only) info
      lua_getglobal (lstate, "xp");
      lua_pushstring (lstate, "tex_size");
      lua_pushnumber (lstate, tex_size);
      lua_settable (lstate, -3);

      qDebug () << "Texture size:" << tex_size << "x" << tex_size;
    }
    lua_pop (lstate, 1);
    
    // Call the update_configuration () callback
    lua_getglobal (lstate, "update_configuration");
    if (lua_isfunction (lstate, -1)) {
      if (lua_pcall (lstate, 0, 0, 0) != 0) {
	qDebug () << "error: could not call update_configuration:"
		  << lua_tostring (lstate, -1);
      }
    }
    lua_pop (lstate, 1);
      
    // Make sure the new size is acceptable on the target screen
    auto geom = dsk.screenGeometry (screen_sbox->value ());
    if (tex_size > geom.width () || tex_size > geom.height ()) {
      // TODO: use the message GUI logging facilities
      qDebug () << "error: texture too big for the screen";
      return;
    }

    // Notify the GLWidget of a new texture size
    glwidget->update_texture_size (tex_size, tex_size);

    // Image on which the frames are painted
    QImage img (tex_size, tex_size, QImage::Format_RGB32);

    // Create a QPainter accessible from Lua
    auto painter = new (lstate, "plstim.qpainter") QPainter;

    // Register the QPainter to avoid collection
    lua_pushlightuserdata (lstate, (void*) painter);
    lua_pushvalue (lstate, -2);
    lua_settable (lstate, LUA_REGISTRYINDEX);

    // Remove the QPainter from the stack
    lua_pop (lstate, 1);

    // Search for a paint_frame () function
    lua_getglobal (lstate, "paint_frame");
    bool pf_is_func = lua_isfunction (lstate, -1);
    lua_pop (lstate, 1);
    
    for (auto p : pages) {
      if (p->type == Page::Type::FRAMES) {
	// Make sure animated frames have updated number of frames
	p->nframes = (int) round ((monitor_rate () / swap_interval)*p->duration/1000.0);
	qDebug () << "Displaying" << p->nframes << "frames for" << p->title;
	qDebug () << "  " << monitor_rate () << "/" << swap_interval;
      }

      // Repaint the page
      if (glwidget_initialised && pf_is_func)
	paint_page (p, img, painter);
    }

    // Mark the QPainter for destruction
    lua_pushlightuserdata (lstate, (void*) painter);
    lua_pushnil (lstate);
    lua_settable (lstate, LUA_REGISTRYINDEX);
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

  this->swap_interval = swap_interval;
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

  // TODO: debug the double free corruption here
  if (lstate != NULL)
    lua_close (lstate);
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
