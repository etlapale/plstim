// plstim/qexperiment.cc – Base class for experimental stimuli


#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;

#include "qexperiment.h"
#include "messagebox.h"
using namespace plstim;


#include <QMenuBar>
#include <QHostInfo>
#include <QtDebug>

using namespace H5;


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
  // Check for an existing page with the same name
  for (auto p : pages) {
    if (! p->title.isEmpty () && p->title == page->title) {
      error (tr ("Duplicate page name detected"));
      return;
    }
  }
  pages.push_back (page);
}

bool
QExperiment::exec ()
{
  cout << "showing windows" << endl;
  win->show ();

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
  QPainter::RenderHints render_hints = QPainter::Antialiasing|QPainter::SmoothPixmapTransform|QPainter::HighQualityAntialiasing;

  switch (page->type) {
  // Single frames
  case Page::Type::SINGLE:
    painter->begin (&img);
    painter->setRenderHints (render_hints);

    lua_getglobal (lstate, "paint_frame");
    lua_pushstring (lstate, page->title.toLocal8Bit ().data ());

    // Fetch the QPainter from the registry
    lua_pushlightuserdata (lstate, (void*) painter);
    lua_gettable (lstate, LUA_REGISTRYINDEX);

    lua_pushnumber (lstate, 0);

    if (! check_lua (lua_pcall (lstate, 3, 0, 0))) {
      painter->end ();
      abort_experiment ();
      return;
    }

    painter->end ();

    //img.save (QString ("page-") + page->title + ".png");
    glwidget->add_fixed_frame (page->title, img);
    break;

  // Multiple frames
  case Page::Type::FRAMES:
    //timer.start ();
    glwidget->delete_animated_frames (page->title);
    //qDebug () << "deleting unamed took: " << timer.elapsed () << " milliseconds" << endl;
    //timer.start ();

    for (int i = 0; i < page->frameCount (); i++) {

      painter->begin (&img);
      painter->setRenderHints (render_hints);

      lua_getglobal (lstate, "paint_frame");
      lua_pushstring (lstate, page->title.toLocal8Bit ().data ());

      // Fetch the QPainter from the registry
      lua_pushlightuserdata (lstate, (void*) painter);
      lua_gettable (lstate, LUA_REGISTRYINDEX);

      lua_pushnumber (lstate, i);

      if (! check_lua (lua_pcall (lstate, 3, 0, 0))) {
	painter->end ();
	abort_experiment ();
	return;
      }

      painter->end ();
#if 0
      QString filename;
      filename.sprintf ("page-%s-%04d.png", qPrintable (page->title), i);
      img.save (filename);
#endif
      glwidget->add_animated_frame (page->title, img);
    }
    //qDebug () << "generating frames took: " << timer.elapsed () << " milliseconds" << endl;
    break;
  }
}

bool
QExperiment::check_lua (int retcode)
{
  switch (retcode) {
  case 0:
    return true;
  case LUA_ERRERR:
    error ("Problem in error handler", lua_tostring (lstate, -1));
    break;
  case LUA_ERRMEM:
    error ("Memory allocation error", lua_tostring (lstate, -1));
    break;
  case LUA_ERRRUN:
    error ("Runtime error", lua_tostring (lstate, -1));
    break;
  case LUA_ERRSYNTAX:
    error ("Syntax error", lua_tostring (lstate, -1));
    break;
  }
  lua_pop (lstate, 1);
  return false;
}

void
QExperiment::run_trial ()
{
  timer.start ();

  // Call the new_trial () callback
  lua_getglobal (lstate, "new_trial");
  if (lua_isfunction (lstate, -1))
    if (! check_lua (lua_pcall (lstate, 0, 0, 0))) {
      abort_experiment ();
      return;
    }

  // Check if there is a paint_frame () function
  lua_getglobal (lstate, "paint_frame");
  bool pf_is_func = lua_isfunction (lstate, -1);
  lua_pop (lstate, 1);

  if (pf_is_func) {
    QImage img (tex_size, tex_size, QImage::Format_RGB32);
    auto painter = new (lstate, "plstim.qpainter") QPainter;

    // Register the QPainter to avoid collection
    lua_pushlightuserdata (lstate, (void*) painter);
    lua_pushvalue (lstate, -2);
    lua_settable (lstate, LUA_REGISTRYINDEX);

    // Remove the QPainter from the stack
    lua_pop (lstate, 1);

    // Paint the per-trial pages
    for (auto p : pages)
      if (p->paint_time == Page::PaintTime::TRIAL)
	paint_page (p, img, painter);
	  
    // Mark the QPainter for destruction
    lua_pushlightuserdata (lstate, (void*) painter);
    lua_pushnil (lstate);
    lua_settable (lstate, LUA_REGISTRYINDEX);
  }

  // Record trial parameters
  for (auto it : trial_offsets) {
    const QString& param_name = it.first;
    auto name = it.first.toUtf8 ();
    size_t offset = it.second;

    lua_getglobal (lstate, name.data ());
    if (lua_isnumber (lstate, -1)) {
      double* pos = (double*) (((char*) trial_record)+offset);
      *pos = lua_tonumber (lstate, -1);
      qDebug () << "storing" << *pos << "for" << param_name;
    }
    lua_pop (lstate, 1);
  }

  current_page = -1;
  //show_page (current_page);
  next_page ();
}

void
QExperiment::show_page (int index)
{
  Page* p = pages[index];

  // Save page parameters
  auto it = record_offsets.find (p->title);
  if (it != record_offsets.end ()) {
    const std::map<QString,size_t>& params = it->second;
    for (auto jt : params) {
      auto param_name = jt.first;
      size_t begin_offset = jt.second;
      if (param_name == "begin") {
	qint64* pos = (qint64*) (((char*) trial_record)+begin_offset);
	qint64 nsecs = timer.nsecsElapsed ();
	*pos = nsecs;
      }
      else if (param_name == "key") {
	// Key will be known at the end of the page
      }
      else {
	/*double* pos = (qint64*) (((char*) trial_record)+begin_offset);
	double value = timer.nsecsElapsed ();
	*pos = nsecs;*/
	qDebug () << "NYI page param";
      }
    }
  }

  switch (p->type) {
  case Page::Type::SINGLE:
    glwidget->show_fixed_frame (p->title);
    
    // Workaround a bug on initial frame after ullscreen
    // Maybe it’s a race condition?
    // TODO: in any cases, it should be tracked and solved
    if (current_trial == 0 && index == 0)
      glwidget->show_fixed_frame (p->title);
    break;
  case Page::Type::FRAMES:
    glwidget->show_animated_frames (p->title);
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
  // Save the page record on HDF5
  if (hf != NULL) {
    hsize_t one = 1;
    DataSpace fspace = dset.getSpace ();
    hsize_t hframe = current_trial;
    fspace.selectHyperslab (H5S_SELECT_SET, &one, &hframe);
    DataSpace mspace (1, &one);
    dset.write (trial_record, *record_type, mspace, fspace);
  }
  
  // End of trial
  if ((int) (current_page + 1) == (int) (pages.size ())) {
    // Next trial
    if (++current_trial < ntrials) {
      run_trial ();
    }
    // End of session
    else {
      if (hf != NULL) {
#if HAVE_EYELINK
	// Choose a name for the local EDF
	auto subject_id = subject_cbox->currentText ();
	auto edf_name = QString ("%1-%2-%3.edf").arg (xp_name).arg (subject_id).arg (session_number);
	check_eyelink (close_data_file ());
	// Buggy Eyelink headers do not care about const
	check_eyelink (receive_data_file ((char*) "", edf_name.toLocal8Bit ().data (), 0));
#endif // HAVE_EYELINK
	hf->flush (H5F_SCOPE_GLOBAL);	// Store everything on file
      }
      glwidget->normal_screen ();
      session_running = false;
    }
  }
  // There is a next page
  else {
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

      // Save pressed key
      if (! page->accepted_keys.empty ()) {
	auto it = record_offsets.find (page->title);
	if (it != record_offsets.end ()) {
	  const std::map<QString,size_t>& params = it->second;
	  auto jt = params.find ("key");
	  if (jt != params.end ()) {
	    size_t key_offset = jt->second;
	    int* pos = (int*) (((char*) trial_record)+key_offset);
	    *pos = evt->key ();
	  }
	}
      }

      next_page ();
    }
  }
}

void
QExperiment::can_run_trial ()
{
  session_running = true;
  run_trial ();
}


static const char* PLSTIM_EXPERIMENT = "plstim::experiment";


static int
l_bin_random (lua_State* lstate)
{
  // Search for the experiment
  lua_pushstring (lstate, PLSTIM_EXPERIMENT);
  lua_gettable (lstate, LUA_REGISTRYINDEX);
  auto xp = (QExperiment*) lua_touserdata (lstate, -1);
  lua_pop (lstate, 1);

  // If we want an array of random numbers
  if (lua_gettop (lstate) >= 1) {
    qDebug () << "bin_random () with argument";
    // Number of elements
    auto size = luaL_checkinteger (lstate, -1);
    lua_pop (lstate, 1);

    // Create the array
    lua_createtable (lstate, size, 0);
    
    // Populate with random numbers
    for (int i = 0; i < size; i++) {
      //lua_pushinteger (lstate, i+1);
      lua_pushboolean (lstate, xp->bin_dist (xp->twister));
      lua_rawseti (lstate, -2, i+1);
      //lua_settable (lstate, -3);
    }
  }
  // If we want a single random number
  else {
    // Generate the random number
    int num = xp->bin_dist (xp->twister);
    lua_pushboolean (lstate, num);
  }
  
  return 1;
}

static int
l_random (lua_State* lstate)
{
  // Search for the experiment
  lua_pushstring (lstate, PLSTIM_EXPERIMENT);
  lua_gettable (lstate, LUA_REGISTRYINDEX);
  auto xp = (QExperiment*) lua_touserdata (lstate, -1);
  lua_pop (lstate, 1);

  // If we want an array of random numbers
  if (lua_gettop (lstate) >= 1) {
    // Number of elements
    auto size = luaL_checkinteger (lstate, -1);
    lua_pop (lstate, 1);

    // Create the array
    lua_createtable (lstate, size, 0);
    
    // Populate with random numbers
    for (int i = 0; i < size; i++) {
      //lua_pushinteger (lstate, i+1);
      lua_pushnumber (lstate, xp->real_dist (xp->twister));
      lua_rawseti (lstate, -2, i+1);
      //lua_settable (lstate, -3);
    }
  }
  // If we want a single random number
  else {
    // Generate the random number
    double num = xp->real_dist (xp->twister);
    lua_pushnumber (lstate, num);
  }
  
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

static int
l_ds2pf (lua_State* lstate)
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
  auto pixs = xp->ds2pf (degs);
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
l_qpainter_draw_path (lua_State* lstate)
{
  auto painter = (QPainter*) lua_touserdata (lstate, 1);
  auto path = (QPainterPath*) lua_touserdata (lstate, 2);

  painter->drawPath (*path);

  return 0;
}

static int
l_qpainter_draw_line (lua_State* lstate)
{
  auto painter = (QPainter*) lua_touserdata (lstate, 1);
  int x1 = luaL_checkint (lstate, 2);
  int y1 = luaL_checkint (lstate, 3);
  int x2 = luaL_checkint (lstate, 4);
  int y2 = luaL_checkint (lstate, 5);

  painter->drawLine (x1, y1, x2, y2);

  return 0;
}

static int
l_qpainter_draw_ellipse (lua_State* lstate)
{
  auto painter = (QPainter*) lua_touserdata (lstate, 1);
  int x = luaL_checkint (lstate, 2);
  int y = luaL_checkint (lstate, 3);
  int width = luaL_checkint (lstate, 4);
  int height = luaL_checkint (lstate, 5);

  painter->drawEllipse (x, y, width, height);

  return 0;
}

static int
l_qpainter_fill_rect (lua_State* lstate)
{
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
  QPainter* painter = (QPainter*) lua_touserdata (lstate, 1);
  QColor col = get_colour (lstate, 2);

  painter->setBrush (col);

  return 0;
}

static int
l_qpainter_set_pen (lua_State* lstate)
{
  QPainter* painter = (QPainter*) lua_touserdata (lstate, 1);

  if (lua_gettop (lstate) == 1)
    painter->setPen (Qt::NoPen);
  else {
    // Colour argument
    if (lua_istable (lstate, 2)) {
      QColor col = get_colour (lstate, 2);
      painter->setPen (col);
    }
    // QPen argument
    else {
      auto pen = (QPen*) luaL_checkudata (lstate, 2, "plstim.qpen");
      painter->setPen (*pen);
    }
  }

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
  {NULL, NULL}
};

static const struct luaL_reg qpainter_lib_m [] = {
  {"draw_ellipse", l_qpainter_draw_ellipse},
  {"draw_line", l_qpainter_draw_line},
  {"draw_path", l_qpainter_draw_path},
  {"fill_rect", l_qpainter_fill_rect},
  {"set_brush", l_qpainter_set_brush},
  {"set_pen", l_qpainter_set_pen},
  {"__gc", l_qpainter_gc},
  {NULL, NULL}
};

static int
l_qpen_new (lua_State* lstate)
{
  if (lua_gettop (lstate) >= 1) {
    auto col = get_colour (lstate, 1);
    new (lstate, "plstim.qpen") QPen (col);
  }
  else {
    new (lstate, "plstim.qpen") QPen;
  }

  return 1;
}

static int
l_qpen_gc (lua_State* lstate)
{
  // Call the C++ destructor
  auto pen = static_cast<QPen*> (lua_touserdata (lstate, 1));
  pen->~QPen ();

  return 0;
}

static int
l_qpen_set_width (lua_State* lstate)
{
  auto pen = (QPen*) lua_touserdata (lstate, 1);
  auto width = luaL_checknumber (lstate, 2);

  pen->setWidthF (width);

  return 0;
}

static const struct luaL_reg qpen_lib_f [] = {
  {"new", l_qpen_new},
  {NULL, NULL}
};

static const struct luaL_reg qpen_lib_m [] = {
  {"set_width", l_qpen_set_width},
  {"__gc", l_qpen_gc},
  {NULL, NULL}
};

static int
l_qpainterpath_new (lua_State* lstate)
{
  // Create a new Lua accessible QPainterPath
  new (lstate, "plstim.qpainterpath") QPainterPath ();
  return 1;
}

static int
l_qpainterpath_gc (lua_State* lstate)
{
  // Call the C++ destructor
  auto path = static_cast<QPainterPath*> (lua_touserdata (lstate, 1));
  path->~QPainterPath ();

  return 0;
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
  {"__gc", l_qpainterpath_gc},
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

  register_class (lstate, "plstim.qpen", qpen_lib_m);
  luaL_openlib (lstate, "qpen", qpen_lib_f, 0);

  return 0;
}

void
QExperiment::open_recent ()
{
  auto action = qobject_cast<QAction*> (sender());
  if (action)
    load_experiment (action->data ().toString ());
}

void
QExperiment::abort_experiment ()
{
  current_page = -1;
  glwidget->normal_screen ();
  session_running = false;
}

void
QExperiment::unload_experiment ()
{
  // No experiment loaded
  if (lstate == NULL)
    return;

  abort_experiment ();

#ifdef HAVE_EYELINK
  if (eyelink_connected)
    close_eyelink_connection ();
#endif

  xp_name.clear ();
  ntrials = 0;
  for (auto p : pages)
    delete p;
  pages.clear ();

  param_spins.clear ();

  delete xp_item;
  if (subject_item)
    delete subject_item;

  if (trial_record != NULL) {
    free (trial_record);
    trial_record = NULL;
  }
  if (record_type != NULL) {
    delete record_type;
    record_type = NULL;
  }
  record_size = 0;
  trial_offsets.clear ();
  record_offsets.clear ();
  xp_keys.clear ();
  if (hf != NULL) {
    hf->close ();
    hf = NULL;
  }

  glwidget->delete_fixed_frames ();
  glwidget->delete_animated_frames ();
  // TODO: also delete shaders. put everything in cleanup ()


  xp_label->setText ("No experiment loaded");

  lua_close (lstate);
  lstate = NULL;
}

bool
QExperiment::load_experiment (const QString& path)
{
  if (unusable) return false;

  // Canonicalise the script path
  QFileInfo fileinfo (path);
  auto script_path = fileinfo.canonicalFilePath ();

  // Store the experiment path in the recent list
  auto rlist = settings->value ("recents").toStringList ();
  rlist.removeAll (script_path);
  rlist.prepend (script_path);
  while (rlist.size () > max_recents)
    rlist.removeLast ();
  settings->setValue ("recents", rlist);
  update_recents ();

  // TODO: cleanup any existing experiment
  unload_experiment ();
  
  // Add the experiment to the settings
  xp_name = fileinfo.fileName ();
  if (xp_name.endsWith (".lua"))
    xp_name = xp_name.left (xp_name.size () - 4);
  settings->beginGroup (QString ("experiments/%1").arg (xp_name));
  if (settings->contains ("path")) {
    if (settings->value ("path").toString () != script_path) {
      error ("Homonymous experiment");
      xp_name.clear ();
      return false;
    }
  }
  else {
    settings->setValue ("path", script_path);
  }
  settings->endGroup ();


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

  // Register random functions
  lua_pushcfunction (lstate, l_random);
  lua_setglobal (lstate, "random");
  lua_pushcfunction (lstate, l_bin_random);
  lua_setglobal (lstate, "bin_random");

  // Register deg2pix ()
  lua_pushcfunction (lstate, l_deg2pix);
  lua_setglobal (lstate, "deg2pix");
  // Register ds2pf ()
  lua_pushcfunction (lstate, l_ds2pf);
  lua_setglobal (lstate, "ds2pf");

  if (! check_lua (luaL_loadfile (lstate, script_path.toLocal8Bit
				  ().data ()))
      || ! check_lua (lua_pcall (lstate, 0, 0, 0))) {
    unload_experiment ();
    return false;
  }

  // Add trial parameters to the record
  lua_getglobal (lstate, "trial_parameters");
  if (lua_istable (lstate, -1)) {
    lua_pushnil (lstate);
    while (lua_next (lstate, -2) != 0) {
      if (lua_isstring (lstate, -2)
	  && lua_istable (lstate, -1)) {
	QString param_name (lua_tostring (lstate, -2));
	trial_offsets[param_name] = record_size;
	record_size += sizeof (double);
	qDebug () << "Adding a trial parameter to the record";
      }
      lua_pop (lstate, 1);
    }
  }
  lua_pop (lstate, 1);
  
  // Check for modules to be loaded
  lua_getglobal (lstate, "modules");
  if (lua_istable (lstate, -1)) {
    size_t mlen = lua_objlen (lstate, -1);
    for (size_t i = 1; i <= mlen; i++) {
      lua_rawgeti (lstate, -1, i);
      if (lua_isstring (lstate, -1)) {
	QString mod_name (lua_tostring (lstate, -1));
#ifdef HAVE_EYELINK
	if (mod_name == "eyelink") {
	  load_eyelink ();
	}
#endif
      }
      lua_pop (lstate, 1);
    }
  }
  lua_pop (lstate, 1);

  // Create the pages defined in the experiment
  lua_getglobal (lstate, "pages");
  if (lua_istable (lstate, -1)) {
    size_t plen = lua_objlen (lstate, -1);
    for (size_t i = 1; i <= plen; i++) {
      lua_rawgeti (lstate, -1, i);
      if (lua_istable (lstate, -1)) {
	// Page information
	QString page_title;
	auto page_type = Page::Type::SINGLE;
	float page_duration = 0;
	std::set<int> accepted_keys;

	// Process each page parameter
	lua_pushnil (lstate);
	while (lua_next (lstate, -2) != 0) {
	  if (lua_isstring (lstate, -2)) {
	    QString param (lua_tostring (lstate, -2));
	    if (param == "name" && lua_isstring (lstate, -1)) {
	      page_title = lua_tostring (lstate, -1);
	    }
	    else if (param == "animated"
		&& lua_isboolean (lstate, -1)) {
	      page_type = lua_toboolean (lstate, -1)
		? Page::Type::FRAMES : Page::Type::SINGLE;
	    }
	    else if (param == "duration"
		     && lua_isnumber (lstate, -1)) {
	      page_duration = lua_tonumber (lstate, -1);
	    }
	    else if (param == "keys"
		     && lua_isstring (lstate, -1)) {
	      QString str (lua_tostring (lstate, -1));
	      auto keys = str.split (" ", QString::SkipEmptyParts);
	      for (auto k : keys) {
		auto it = key_mapping.find (k);
		if (it == key_mapping.end ()) {
		  qDebug () << "error: no mapping for key " << k;
		}
		else {
		  accepted_keys.insert (it->second);
		  xp_keys.insert (k);
		}
	      }
	    }
	  }
	  lua_pop (lstate, 1);
	}

	// Create a new page and add it to the experiment
	auto page = new Page (page_type, page_title);
	page->duration = page_duration;
	page->accepted_keys = accepted_keys;
	add_page (page);

	// Compute required memory for the page
	record_offsets[page_title]["begin"] = record_size;
	record_size += sizeof (qint64);	// Start page presentation
	if (! accepted_keys.empty ()) {
	  record_offsets[page_title]["key"] = record_size;
	  record_size += sizeof (int);	// Pressed key
	}
      }
      lua_pop (lstate, 1);
    }
  }

  // Define the trial record
  if (record_size) {
    qDebug () << "record size set to" << record_size;
    trial_record = malloc (record_size);
    record_type = new CompType (record_size);
    // Add trial info to the record
    for (auto it : trial_offsets) {
      qDebug () << "  Trial for" << it.first << "at" << it.second;
      record_type->insertMember (it.first.toUtf8 ().data (), it.second, PredType::NATIVE_DOUBLE);
    }
    // Add pages info to the record
    QString fmt ("%1_%2");
    for (auto p : pages) {
      auto page_name = p->title;
      for (auto param : record_offsets[page_name]) {
	auto param_name = param.first;
	auto offset = param.second;
	H5::DataType param_type;
	if (param_name == "begin")
	  param_type = PredType::NATIVE_LONG;
	else if (param_name == "key")
	  param_type = PredType::NATIVE_INT;
	else
	  param_type = PredType::NATIVE_DOUBLE;
	qDebug () << "  Record for page" << page_name << "param" << param_name << "at" << offset;
	record_type->insertMember (fmt.arg (page_name).arg (param_name).toUtf8 ().data (), offset, param_type);
      }
    }
    qDebug () << "Trial record size:" << record_size;
  }


  // Setup an experiment item in the left toolbox
  xp_item = new QWidget;
  auto flayout = new QFormLayout;
  // Experiment name flayout->addRow ("Experiment", new QLabel (xp_name)); // Number of trials
  ntrials_spin = new QSpinBox;
  ntrials_spin->setRange (0, 8000);
  connect (ntrials_spin, SIGNAL (valueChanged (int)),
	   this, SLOT (set_trial_count (int)));
  flayout->addRow ("Number of trials", ntrials_spin);
  // Add spins for experiment specific parameters
  lua_getglobal (lstate, "parameters");
  if (lua_istable (lstate, -1)) {
    lua_pushnil (lstate);
    while (lua_next (lstate, -2) != 0) {
      if (lua_isstring (lstate, -2)
	  && lua_istable (lstate, -1)) {
	QString param_name (lua_tostring (lstate, -2));

	lua_rawgeti (lstate, -1, 1);
	QString param_desc (lua_tostring (lstate, -1));
	lua_pop (lstate, 1);

	lua_rawgeti (lstate, -1, 2);
	auto param_unit = QString (" %1").arg(lua_tostring (lstate, -1));
	lua_pop (lstate, 1);

	auto spin = new QDoubleSpinBox;
	spin->setRange (-8000, 8000);
	spin->setSuffix (param_unit);
	spin->setProperty ("plstim_param_name", param_name);
	connect (spin, SIGNAL (valueChanged (double)),
		 this, SLOT (xp_param_changed (double)));
	flayout->addRow (param_desc, spin);
	
	param_spins[param_name] = spin;
      }
      lua_pop (lstate, 1);
    }
  }
  lua_pop (lstate, 1);
  // Finish the experiment item
  xp_item->setLayout (flayout);
  tbox->addItem (xp_item, "Experiment");

  // Fetch experiment parameters
  lua_getglobal (lstate, "ntrials");
  if (lua_isnumber (lstate, -1)) {
    set_trial_count ((int) lua_tonumber (lstate, -1));
  }
  lua_pop (lstate, 1);
  for (auto it : param_spins) {
    lua_getglobal (lstate, it.first.toUtf8().data ());
    if (lua_isnumber (lstate, -1))
      it.second->setValue (lua_tonumber (lstate, -1));
    lua_pop (lstate, 1);
  }

  xp_label->setText (script_path);


  // Setup a subject item in the left toolbox
  subject_item = new QWidget;
  flayout = new QFormLayout;
  subject_cbox = new QComboBox;
  subject_cbox->addItem (tr ("None"));
  flayout->addRow ("Subject", subject_cbox);
  connect (subject_cbox, SIGNAL (currentIndexChanged (const QString&)),
	   this, SLOT (subject_changed (const QString&)));
  subject_databutton = new QPushButton ( tr( "Subject data file"));
  subject_databutton->setDisabled (true);
  connect (subject_databutton, SIGNAL (clicked ()),
	   this, SLOT (select_subject_datafile ()));
  flayout->addRow ("Data file", subject_databutton);
  subject_item->setLayout (flayout);
  tbox->addItem (subject_item, "Subject");

  // Load the available subject ids for the experiment
  QString subject_path ("experiments/%1/subjects");
  settings->beginGroup (subject_path.arg (xp_name));
  for (auto s : settings->childKeys ())
    subject_cbox->addItem (s);
  settings->endGroup ();

  // Prepare for running the experiment
  tbox->setCurrentWidget (subject_item);

  // Initialise the experiment
  setup_updated ();

  return true;
}

void
QExperiment::xp_param_changed (double value)
{
  auto spin = qobject_cast<QDoubleSpinBox*> (sender());
  if (spin) {
    auto param = spin->property ("plstim_param_name").toString ();
    lua_pushnumber (lstate, value);
    lua_setglobal (lstate, param.toUtf8 ().data ());
  }
}

void
QExperiment::set_trial_count (int num_trials)
{
  ntrials = num_trials;
  ntrials_spin->setValue (ntrials);
}

static const QRegExp session_name_re ("session_[0-9]+");

herr_t
find_session_maxi (hid_t group_id, const char * dset_name, void* data)
{
  (void) group_id;	// Unused

  QString dset (dset_name);

  // Check if the dataset is a session
  if (session_name_re.exactMatch (dset)) {
    int* maxi = (int*) data;
    int num = dset.right (dset.size () - 8).toInt ();
    if (num > *maxi)
      *maxi = num;
  }
  return 0;
}


void
QExperiment::init_session ()
{
  current_trial = 0;

  // Check if a subject datafile is opened
  if (hf != NULL) {
    // Parse the HDF5 datasets to get a block number
    int session_maxi = 0;
    int idx = 0;
    hf->iterateElems ("/", &idx, find_session_maxi, &session_maxi);
    session_number = session_maxi + 1;

    // Give a number to the current block
    auto session_name = QString ("session_%1").arg (session_number);

    // Create a new dataset for the block/session
    hsize_t htrials = ntrials;
    DataSpace dspace (1, &htrials);
    dset = hf->createDataSet (session_name.toUtf8 ().data (),
			      *record_type, dspace);
    
    // Save subject ID
    auto subject = subject_cbox->currentText ().toUtf8 ();
    StrType str_type (PredType::C_S1, subject.size ());
    DataSpace scalar_space (H5S_SCALAR);
    dset.createAttribute ("subject", str_type, scalar_space)
      .write (str_type, subject.data ());

    // Save machine information
    auto hostname = QHostInfo::localHostName ().toUtf8 ();
    StrType sysname_type (PredType::C_S1, hostname.size ());
    dset.createAttribute ("hostname", sysname_type, scalar_space)
      .write (sysname_type, hostname.data ());

    // Save current date and time
    qint64 now = QDateTime::currentMSecsSinceEpoch ();
    dset.createAttribute ("datetime", PredType::STD_U64LE, scalar_space)
      .write (PredType::NATIVE_UINT64, &now);

    // Save key mapping
    size_t rowsize = sizeof (int) + sizeof (char*);
    char* km = new char[xp_keys.size () * rowsize];
    unsigned int i = 0;
    for (auto k : xp_keys) {
      auto utf_key = k.toUtf8 ();

      int* code = (int*) (km+rowsize*i);
      char** name = (char**) (km+rowsize*i+sizeof (int));

      *code = key_mapping[k];
      *name = new char[utf_key.size ()+1];
      strcpy (*name, utf_key.data ());
      i++;
    }
    StrType keys_type (PredType::C_S1, H5T_VARIABLE);
    keys_type.setCset (H5T_CSET_UTF8);
    hsize_t nkeys = xp_keys.size ();
    DataSpace kspace (1, &nkeys);

    CompType km_type (rowsize);
    km_type.insertMember ("code", 0, PredType::NATIVE_INT);
    km_type.insertMember ("name", sizeof (int), keys_type);
    dset.createAttribute ("keys", km_type, kspace)
      .write (km_type, km);

    hf->flush (H5F_SCOPE_GLOBAL);
    for (i = 0; i < nkeys; i++) {
      char** name = (char**) (km+rowsize*i+sizeof (int));
      //free (*name);
      delete [] (*name);
    }
    delete [] km;

#ifdef HAVE_EYELINK
    // Note, we do not record EyeTracker session if we 
    // do not have an HDF5 datafile were to reference it
    auto edf_name = QString ("plst-%1.edf").arg (session_number);
    qDebug () << "storing in temporary EDF" << edf_name;
    open_data_file (edf_name.toLocal8Bit ().data ());
    // Data stored in the EDF file
    eyecmd_printf ("file_sample_data = LEFT,RIGHT,GAZE,GAZERES,AREA,STATUS");
    eyecmd_printf ("file_event_filter = LEFT,RIGHT,FIXATION,SACCADE,BLINK,MESSAGE");
#endif // HAVE_EYELINK
  }

  // Run the calibration (no need to wait OpenGL full screen)
  calibrate_eyelink ();
}

void
QExperiment::run_session ()
{
  init_session ();
  splitter_state = splitter->saveState ();
  glwidget->full_screen (off_x_edit->value (), off_y_edit->value ());
}

void
QExperiment::run_session_inline ()
{
  init_session ();
  glwidget->setFocus (Qt::OtherFocusReason);
  can_run_trial ();
}

bool
QExperiment::clear_screen ()
{
  return false;
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
  QSize sz = win->size ();
  settings->setValue ("gui_width", sz.width ());
  settings->setValue ("gui_height", sz.height ());
  settings->setValue ("splitter", splitter->saveState ());
}

QSpinBox*
QExperiment::make_setup_spin (int min, int max, const char* suffix)
{
  auto spin = new QSpinBox;
  spin->setRange (min, max);
  spin->setSuffix (suffix);
  connect (spin, SIGNAL (valueChanged (int)),
	   this, SLOT (setup_param_changed ()));
  return spin;
}

QExperiment::QExperiment (int & argc, char** argv)
  : app (argc, argv),
    save_setup (false),
    glwidget_initialised (false),
    bin_dist (0, 1),
    real_dist (0, 1),
    unusable (false),
    trial_record (NULL),
    record_size (0),
    hf (NULL)
{
  plstim::initialise ();

  win = new QMainWindow;
  lstate = NULL;
  subject_item = NULL;
  record_type = NULL;

  session_running = false;

#ifdef HAVE_EYELINK
  eyelink_connected = false;
#ifdef DUMMY_EYELINK
  eyelink_dummy = true;
#else
  eyelink_dummy = false;
#endif
#endif
  
  // Initialise the random number generator
  // TODO: make that configurable
  twister.seed (time (NULL));
  for (int i = 0; i < 10000; i++)
    (void) bin_dist (twister);


  // Mapping of key names to keys for Lua
  key_mapping["down"] = Qt::Key_Down;
  key_mapping["left"] = Qt::Key_Left;
  key_mapping["right"] = Qt::Key_Right;
  key_mapping["up"] = Qt::Key_Up;
  

  // Get the experimental setup

  glwidget = NULL;

  splitter = new QSplitter;

  // Create a basic menu
  xp_menu = win->menuBar ()->addMenu (tr ("&Experiment"));
  // 
  auto action = xp_menu->addAction ("&Open");
  action->setShortcut (tr ("Ctrl+O"));
  action->setStatusTip (tr ("&Open an experiment"));
  connect (action, SIGNAL (triggered ()), this, SLOT (open ()));

  // Run an experiment in full screen
  action = xp_menu->addAction ("&Run");
  action->setShortcut (tr ("Ctrl+R"));
  action->setStatusTip (tr ("&Run the experiment"));
  connect (action, SIGNAL (triggered ()), this, SLOT (run_session ()));

  // Run an experiment inside the main window
  action = xp_menu->addAction ("Run &inline");
  action->setShortcut (tr ("Ctrl+Shift+R"));
  action->setStatusTip (tr ("&Run the experiment inside the window"));
  connect (action, SIGNAL (triggered ()), this, SLOT (run_session_inline ()));

  // Close the current simulation
  close_action = xp_menu->addAction ("&Close");
  close_action->setShortcut (tr ("Ctrl+W"));
  close_action->setStatusTip (tr ("&Close the experiment"));
  connect (close_action, SIGNAL (triggered ()), this, SLOT (unload_experiment ()));
  xp_menu->addSeparator ();

  // Maximum number of recent experiments displayed
  max_recents = 5;
  recent_actions = new QAction*[max_recents];
  for (int i = 0; i < max_recents; i++) {
    recent_actions[i] = new QAction (xp_menu);
    recent_actions[i]->setVisible (false);
    xp_menu->addAction (recent_actions[i]);
    connect (recent_actions[i], SIGNAL (triggered ()),
	     this, SLOT (open_recent ()));
  }
  xp_menu->addSeparator ();

  // Terminate the program
  action = xp_menu->addAction ("&Quit");
  action->setShortcut(tr("Ctrl+Q"));
  action->setStatusTip(tr("&Quit the program"));
  connect (action, SIGNAL (triggered ()), this, SLOT (quit ()));

  // Setup menu
  auto menu = win->menuBar ()->addMenu (tr ("&Setup"));
  action = menu->addAction (tr ("&New"));
  action->setStatusTip (tr ("&Create a new setup"));
  connect (action, SIGNAL (triggered ()), this, SLOT (new_setup ()));

  // Subject menu
  menu = win->menuBar ()->addMenu (tr ("&Subject"));
  action = menu->addAction (tr ("&New"));
  action->setShortcut (tr ("Ctrl+N"));
  action->setStatusTip (tr ("&Create a new subject"));
  connect (action, SIGNAL (triggered ()), this, SLOT (new_subject ()));

  // Top toolbar
  //toolbar = new QToolBar ("Simulation");
  //action = 

  // Left toolbox
  tbox = new QToolBox;
  splitter->addWidget (tbox);

  // Horizontal splitter (top: GLwidget, bottom: message box)
  hsplitter = new QSplitter (Qt::Vertical);
  hsplitter->addWidget (splitter);
  logtab = new QTabWidget;
  msgbox = new MyMessageBox;
  logtab->addTab (msgbox, "Messages");
  hsplitter->addWidget (logtab);

  win->setCentralWidget (hsplitter);

  // Status bar with current experiment name
  xp_label = new QLabel ("No experiment loaded");
  win->statusBar ()->addWidget (xp_label);

  // Experimental setup
  setup_item = new QWidget;
  setup_cbox = new QComboBox;
  connect (setup_cbox, SIGNAL (currentIndexChanged (const QString&)),
	   this, SLOT (update_setup ()));
  screen_sbox = new QSpinBox;
  connect (screen_sbox, SIGNAL (valueChanged (int)),
	   this, SLOT (setup_param_changed ()));
  // Set minimum to mode 13h, and maximum to 4320p
  off_x_edit = make_setup_spin (0, 7680, " px");
  off_y_edit = make_setup_spin (0, 4320, " px");
  res_x_edit = make_setup_spin (320, 7680, " px");
  res_y_edit = make_setup_spin (200, 4320, " px");
  phy_width_edit = make_setup_spin (10, 8000, " mm");
  phy_height_edit = make_setup_spin (10, 8000, " mm");
  dst_edit = make_setup_spin (10, 8000, " mm");
  lum_min_edit = make_setup_spin (1, 800, " cd/m²");
  lum_max_edit = make_setup_spin (1, 800, " cd/m²");
  refresh_edit = make_setup_spin (1, 300, " Hz"); 

  auto flayout = new QFormLayout;
  flayout->addRow ("Setup name", setup_cbox);
  flayout->addRow ("Screen", screen_sbox);
  flayout->addRow ("Horizontal offset", off_x_edit);
  flayout->addRow ("Vertical offset", off_y_edit);
  flayout->addRow ("Horizontal resolution", res_x_edit);
  flayout->addRow ("Vertical resolution", res_y_edit);
  flayout->addRow ("Physical width", phy_width_edit);
  flayout->addRow ("Physical height", phy_height_edit);
  flayout->addRow ("Distance", dst_edit);
  flayout->addRow ("Minimum luminance", lum_min_edit);
  flayout->addRow ("Maximum luminance", lum_max_edit);
  flayout->addRow ("Refresh rate", refresh_edit);
  setup_item->setLayout (flayout);
  tbox->addItem (setup_item, "Setup");
  
  // Check for OpenGL
  if (! QGLFormat::hasOpenGL ()) {
    error ("OpenGL not found");
    unusable = true;
    return;
  }

  /*
  auto gdsk = dsk.screenGeometry ();
  QString dsk_msg ("Found %1 screens in a %2 desktop of %3×%4");
  error (dsk_msg.arg (dsk.screenCount ()).arg (dsk.isVirtualDesktop () ? "virtual" : "normal").arg (gdsk.width ()).arg (gdsk.height ()));
  */

  auto flags = QGLFormat::openGLVersionFlags ();
  if (! (flags & QGLFormat::OpenGL_Version_3_0)) {
    QString found ("unknown");
    if (flags & QGLFormat::OpenGL_Version_2_1)
      found = "2.1";
    else if (flags & QGLFormat::OpenGL_Version_2_0)
      found = "2.0";
    else if (flags & QGLFormat::OpenGL_Version_1_5)
      found = "1.5";
    else if (flags & QGLFormat::OpenGL_Version_1_4)
      found = "1.4";
    else if (flags & QGLFormat::OpenGL_Version_1_3)
      found = "1.3";
    else if (flags & QGLFormat::OpenGL_Version_1_2)
      found = "1.2";
    else if (flags & QGLFormat::OpenGL_Version_1_1)
      found = "1.1";
    error (QString ("OpenGL 3.0 not supported (limited to %1)").arg (found));
    unusable = true;
    return;
  }

  
  // Create an OpenGL widget
  QGLFormat fmt;
  fmt.setDoubleBuffer (true);
  fmt.setVersion (3, 0);
  fmt.setDepth (false);
  fmt.setSwapInterval (1);
  set_glformat (fmt);
  // Show OpenGL version in GUI
  QString glfmt ("%1.%2");
  flayout->addRow ("OpenGL", new QLabel (glfmt.arg (glwidget->format ().majorVersion ()).arg (glwidget->format ().minorVersion ())));
  // Check for double buffering
  if (! glwidget->format ().doubleBuffer ()) {
    error ("Missing double buffering support");
    unusable = true;
    return;
  }
  splitter->addWidget (glwidget);


  // Try to fetch back setup
  settings = new QSettings;
  settings->beginGroup ("setups");
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
    res_x_edit->setValue (geom.width ());
    res_y_edit->setValue (geom.height ());
    settings->endGroup ();
  }

  // Use an existing setup
  else {
    // Add all the setups to the combo box
    for (auto g : groups) {
      if (g != "General") {
	setup_cbox->addItem (g);
      }
    }
    settings->endGroup ();

    // TODO: check if there was a ‘last’ setup
    
    // Restore setup
    update_setup ();
  }

  // Restore GUI state
  if (settings->contains ("gui_width"))
    win->resize (settings->value ("gui_width").toInt (),
		settings->value ("gui_height").toInt ());
  if (settings->contains ("splitter")) {
    qDebug () << "restoring splitter state from config";
    splitter->restoreState (settings->value ("splitter").toByteArray ());
  }

  // Set recent experiments
  update_recents ();

  // Constrain the screen selector
  screen_sbox->setMinimum (0);
  screen_sbox->setMaximum (dsk.screenCount () - 1);

  save_setup = true;

  // Close event termination
  connect (&app, SIGNAL (aboutToQuit ()),
	   this, SLOT (about_to_quit ()));

  // Make sure the timer is monotonic
  if (! timer.isMonotonic ())
    error (tr ("No monotonic timer available on this platform"));
}

void
QExperiment::error (const QString& msg, const QString& desc)
{
  msgbox->add (new Message (MESSAGE_TYPE_ERROR, msg));
}

void
QExperiment::new_subject ()
{
  tbox->setCurrentWidget (subject_item);
  subject_cbox->setEditable (true);
  auto le = new MyLineEdit;
  subject_cbox->setLineEdit (le);
  le->clear ();

  connect (le, SIGNAL (returnPressed ()),
	   this, SLOT (new_subject_validated ()));
  connect (le, SIGNAL (escapePressed ()),
	   this, SLOT (new_subject_cancelled ()));

  QRegExp rx ("[a-zA-Z0-9\\-_]{1,10}");
  le->setValidator (new QRegExpValidator (rx));
  subject_cbox->setFocus (Qt::OtherFocusReason);
}

void
QExperiment::new_setup ()
{
  tbox->setCurrentWidget (setup_item);
  setup_cbox->setEditable (true);
  auto le = new MyLineEdit;
  setup_cbox->setLineEdit (le);
  le->clear ();

  connect (le, SIGNAL (returnPressed ()),
	   this, SLOT (new_setup_validated ()));
  connect (le, SIGNAL (escapePressed ()),
	   this, SLOT (new_setup_cancelled ()));

  QRegExp rx ("[a-zA-Z0-9\\-_]{1,10}");
  le->setValidator (new QRegExpValidator (rx));
  setup_cbox->setFocus (Qt::OtherFocusReason);
}

void
QExperiment::new_setup_validated ()
{
  auto le = qobject_cast<MyLineEdit*> (sender());
  auto setup_name = le->text ();
  auto setup_path = QString ("setups/%1").arg (setup_name);
  
  // Make sure the setup name is not already present
  if (settings->contains (setup_path)) {
    error (tr ("Setup name already existing"));
    return;
  }

  setup_cbox->setEditable (false);
}

void
QExperiment::new_setup_cancelled ()
{
  auto le = qobject_cast<MyLineEdit*> (sender());
  if (le) {
    le->clear ();
    setup_cbox->setEditable (false);
  }
}

void
QExperiment::new_subject_validated ()
{
  auto le = qobject_cast<MyLineEdit*> (sender());
  auto subject_id = le->text ();
  auto subject_path = QString ("experiments/%1/subjects/%2")
    .arg (xp_name).arg (subject_id);

  // Make sure the subject ID is not already present
  // and is not the ‘no subject’ option
  if (subject_cbox->currentIndex () == 0
      || settings->contains (subject_path)) {
    error (tr ("Subject ID already existing"));
    return;
  }

  subject_cbox->setEditable (false);

  // Set a default datafile for the subject
  auto subject_datafile = QString ("%1-%2.h5").arg (xp_name).arg (subject_id);
  QFileInfo fi (subject_datafile);
  int flags = H5F_ACC_EXCL;
  if (fi.exists ())
    flags = H5F_ACC_RDWR;

  // Set the datafile button info
  subject_databutton->setText (fi.fileName ());
  subject_databutton->setToolTip (fi.absoluteFilePath ());

  // Create the HDF5 file
  // TODO: handle HDF5 exceptions here
  if (hf != NULL) {
    hf->close ();
    hf = NULL;
  }
  hf = new H5File (subject_datafile.toLocal8Bit ().data (), flags);

  // Create a new subject
  settings->setValue (subject_path, fi.absoluteFilePath ());
}

void
QExperiment::new_subject_cancelled ()
{
  auto le = qobject_cast<MyLineEdit*> (sender());
  if (le) {
    le->clear ();
    subject_cbox->setEditable (false);
  }
}

void
QExperiment::select_subject_datafile ()
{
  QFileDialog dialog (win, tr ("Select datafile"), last_datafile_dir);
  dialog.setDefaultSuffix (".h5");
  dialog.setNameFilter (tr ("Subject data (*.h5 *.hdf);;All files (*)"));
  if (dialog.exec () == QDialog::Accepted) {
    last_datafile_dir = dialog.directory ().absolutePath ();
    set_subject_datafile (dialog.selectedFiles ().at (0));
  }
}

void
QExperiment::subject_changed (const QString& subject_id)
{
  // Selection removal
  if (subject_cbox->currentIndex () == 0) {
    subject_databutton->setText (tr ("Subject data file"));
    subject_databutton->setToolTip (QString ());
    subject_databutton->setDisabled (true);
  }
  else {
    auto subject_path = QString ("experiments/%1/subjects/%2")
      .arg (xp_name).arg (subject_id);

    auto datafile = settings->value (subject_path).toString ();
    QFileInfo fi (datafile);

    // Make sure the datafile still exists
    if (! fi.exists ())
      error (tr ("Subject data file is missing"));
    // Open the HDF5 datafile
    else {
      // TODO: handle HDF5 exceptions here
      if (hf != NULL) {
	hf->close ();
	hf = NULL;
      }
      hf = new H5File (datafile.toLocal8Bit ().data (),
		       H5F_ACC_RDWR);
    }

    subject_databutton->setText (fi.fileName ());
    subject_databutton->setToolTip (fi.absoluteFilePath ());
    subject_databutton->setDisabled (false);
  }
}

void
QExperiment::set_subject_datafile (const QString& path)
{
  QFileInfo fi (path);
  subject_databutton->setText (fi.fileName ());
  subject_databutton->setToolTip (path);
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
    else {
      set_swap_interval (1);
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
    if (lua_isfunction (lstate, -1))
      if (! check_lua (lua_pcall (lstate, 0, 0, 0))) {
	abort_experiment ();
	return;
      }
      
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
      // Assume that setup is not updated during trials, so
      // that paint_frame () is called by run_trial () for
      // per-trial pages
      if (glwidget_initialised && pf_is_func
	  && p->paint_time != Page::PaintTime::TRIAL)
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
  qDebug () << "normal screen restored";
  if (glwidget->parent () != splitter) {
    splitter->addWidget (glwidget);
    splitter->restoreState (splitter_state);
  }
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

  glwidget = new MyGLWidget (glformat, win);

  splitter->addWidget (glwidget);

  // Re-add the glwidget to the splitter when fullscreen ends
  connect (glwidget, SIGNAL (normal_screen_restored ()),
	   this, SLOT (normal_screen_restored ()));
  connect (glwidget, SIGNAL (gl_initialised ()),
	   this, SLOT (glwidget_gl_initialised ()));
  connect (glwidget, SIGNAL (can_run_trial ()),
	   this, SLOT (can_run_trial ()));
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

void
QExperiment::update_recents ()
{
  auto rlist = settings->value ("recents").toStringList ();
  int i = 0;
  QString label ("&%1 %2");
  for (auto path : rlist) {
    QFileInfo fi (path);
    QString name = fi.fileName ();
    if (name.endsWith (".lua"))
      name = name.left (name.size () - 4);
    recent_actions[i]->setText (label.arg (i+1).arg (name));
    recent_actions[i]->setData (path);
    recent_actions[i]->setVisible (true);
    i++;
  }
  while (i < max_recents)
    recent_actions[i++]->setVisible (false);
}

void
QExperiment::update_converters ()
{
  distance = dst_edit->value ();

  float hres = (float) res_x_edit->value ()
    / phy_width_edit->value ();
  float vres = (float) res_y_edit->value ()
    / phy_height_edit->value ();
  //float err = fabsf ((hres-vres) / (hres+vres));

  // TODO: restablish messages for resolutions
#if 0
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
#endif

  px_mm = (hres+vres)/2.0;

  // TODO: restablish resolution messages
#if 0

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
#endif
}

QExperiment::~QExperiment ()
{
  delete settings;
  delete glwidget;
  delete win;

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

  settings->beginGroup ("setups");
  settings->beginGroup (setup_cbox->currentText ());
  settings->setValue ("scr", screen_sbox->value ());
  settings->setValue ("off_x", off_x_edit->value ());
  settings->setValue ("off_y", off_y_edit->value ());
  settings->setValue ("res_x", res_x_edit->value ());
  settings->setValue ("res_y", res_y_edit->value ());
  settings->setValue ("phy_w", phy_width_edit->value ());
  settings->setValue ("phy_h", phy_height_edit->value ());
  settings->setValue ("dst", dst_edit->value ());
  settings->setValue ("lmin", lum_min_edit->value ());
  settings->setValue ("lmax", lum_max_edit->value ());
  settings->setValue ("rate", refresh_edit->value ());
  settings->endGroup ();
  settings->endGroup ();

  // Emit the signal
  setup_updated ();
}

void
QExperiment::update_setup ()
{
  auto sname = setup_cbox->currentText ();
  qDebug () << "restoring setup for" << sname;

  settings->beginGroup ("setups");
  settings->beginGroup (sname);
  screen_sbox->setValue (settings->value ("scr").toInt ());
  off_x_edit->setValue (settings->value ("off_x").toInt ());
  off_y_edit->setValue (settings->value ("off_y").toInt ());
  res_x_edit->setValue (settings->value ("res_x").toInt ());
  res_y_edit->setValue (settings->value ("res_y").toInt ());
  phy_width_edit->setValue (settings->value ("phy_w").toInt ());
  phy_height_edit->setValue (settings->value ("phy_h").toInt ());
  dst_edit->setValue (settings->value ("dst").toInt ());
  lum_min_edit->setValue (settings->value ("lmin").toInt ());
  lum_max_edit->setValue (settings->value ("lmax").toInt ());
  refresh_edit->setValue (settings->value ("rate").toInt ());
  settings->endGroup ();
  settings->endGroup ();

  setup_updated ();
}

float
QExperiment::monitor_rate () const
{
  return (float) refresh_edit->value ();
}

void
QExperiment::open ()
{
  QFileDialog dialog (win, tr ("Open experiment"), last_dialog_dir);
  dialog.setFileMode (QFileDialog::ExistingFile);
  dialog.setNameFilter (tr ("Experiment (*.lua);;All files (*)"));

  if (dialog.exec () == QDialog::Accepted) {
    last_dialog_dir = dialog.directory ().absolutePath ();
    load_experiment (dialog.selectedFiles ().at (0));
  }
}
