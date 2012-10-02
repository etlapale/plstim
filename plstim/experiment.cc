// plstim/experiment.cc – Base class for experimental stimuli

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_XRANDR

#include <png.h>

#include <iomanip>
#include <iostream>
#include <sstream>
using namespace std;


#include "experiment.h"
using namespace plstim;



static bool
assert_gl_error (const std::string& msg)
{
  auto ret = glGetError ();
  if (ret != GL_NO_ERROR) {
    cerr << "could not " << msg
	 << " (" << hex << ret << dec << ')' << endl;
    exit(1);
  }
  return true;
}


bool
Experiment::make_native_window (EGLNativeDisplayType nat_dpy,
				EGLDisplay egl_dpy,
				int width, int height,
				bool fullscreen,
				const char* title,
				EGLNativeWindowType *window,
				EGLConfig *conf)
{
  // Required characteristics
  EGLint egl_attr[] = {
    EGL_BUFFER_SIZE, 24,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  // Find a matching config
  EGLConfig config;
  EGLint num_egl_configs;
  if (! eglChooseConfig (egl_dpy, egl_attr, &config,
			 1, &num_egl_configs)
      || num_egl_configs != 1
      || config == 0) {
    fprintf (stderr, "could not choose an EGL config\n");
    return false;
  }

  // Get the default screen and root window
  Display *dpy = (Display*) nat_dpy;
  Window root = DefaultRootWindow (dpy);

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


  // Fetch visual information for the configuration
  EGLint vid;
  int num_visuals;
  if (! eglGetConfigAttrib (egl_dpy, config,
			    EGL_NATIVE_VISUAL_ID, &vid)) {
    fprintf (stderr, "could not get native visual id\n");
    return false;
  }
  XVisualInfo xvis;
  XVisualInfo *xvinfo;
  xvis.visualid = vid;
  xvinfo = XGetVisualInfo (dpy, VisualIDMask, &xvis, &num_visuals);
  if (num_visuals == 0) {
    fprintf (stderr, "could not get the X visuals\n");
    return false;
  }

  // Set the window attributes
  XSetWindowAttributes xattr;
  unsigned long mask;
  Window win;
  xattr.background_pixel = 0;
  xattr.border_pixel = 0;
  xattr.colormap = XCreateColormap (dpy, root,
				    xvinfo->visual, AllocNone);
  xattr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
  mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
  win = XCreateWindow (dpy, root, win_x, win_y, width, height, 0,
			xvinfo->depth, InputOutput, xvinfo->visual,
			mask, &xattr);
  *window = (EGLNativeWindowType) win;

  // Set X hints and properties
  XStoreName (dpy, win, title);

  XMapWindow (dpy, win);
  XFree (xvinfo);

  // Make the window fullscreen
  if (fullscreen) {
    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom fs_atm = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = fs_atm;
    xev.xclient.data.l[2] = 0;
    XSendEvent (dpy, DefaultRootWindow (dpy), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
  }

  *conf = config;
  return true;
}

bool
Experiment::run_session ()
{
  for (current_trial = 0; current_trial < ntrials; current_trial++)
    if (! run_trial ())
      return false;
  return true;
}

bool
Experiment::wait_any_key ()
{
  bool pressed = false;
  XEvent evt;
  while (! pressed) {
    XNextEvent ((Display*) nat_dpy, &evt);
    if (evt.type == KeyPress)
      pressed = true;
  }
  return true;
}

bool
Experiment::wait_for_key (const std::vector<KeySym>& accepted_keys,
			  KeySym* pressed_key)
{
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
  return false;
}

bool
Experiment::gen_frame (const std::string& frame_name)
{
  glGenTextures (1, &special_frames[frame_name]);
  assert_gl_error ("generate textures");
  return true;
}

bool
Experiment::copy_to_texture (const GLvoid* data, GLuint dest)
{
    // Create an OpenGL texture for the frame
  glBindTexture (GL_TEXTURE_2D, dest);
  if (glGetError () != GL_NO_ERROR) {
    cerr << "error: could not bind a texture" << endl;
    return false;
  }
  GLint gltype = GL_RGBA;
  glTexImage2D (GL_TEXTURE_2D, 0, gltype, tex_width, tex_height,
		0, gltype, GL_UNSIGNED_BYTE, data);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  if (glGetError () != GL_NO_ERROR) {
    cerr << "error: could not set texture data" << endl;
    return false;
  }
  return true;
}

bool
Experiment::show_frame (const std::string& frame_name)
{
  // Set the current frame
  glBindTexture (GL_TEXTURE_2D, special_frames[frame_name]);
  glUniform1i (texloc, 0);

  // Draw the frame with triangles
  glDrawArrays (GL_TRIANGLES, 0, 6);
  
  // Swap
  eglSwapBuffers (egl_dpy, sur);

  return true;
}

bool
Experiment::show_frames ()
{
  for (int i = 0; i < nframes; i++) {
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
  }

  return true;
}

bool
Experiment::clear_screen ()
{
  glClear (GL_COLOR_BUFFER_BIT);
  eglSwapBuffers (egl_dpy, sur);

  return true;
}


static EGLNativeDisplayType
open_native_display (void* param)
{
  return (EGLNativeDisplayType) XOpenDisplay ((char*) param);
}


bool
Experiment::egl_init (int win_width, int win_height, bool fullscreen,
		      const std::string& win_title,
		      int stim_width, int stim_height)
{
  tex_width = 1 << ((int) log2f (stim_width));
  tex_height = 1 << ((int) log2f (stim_height));
  if (tex_width < stim_width)
    tex_width <<= 1;
  if (tex_height < stim_height)
    tex_height <<= 1;

  // Setup X11
  nat_dpy = open_native_display (NULL);

  // Setup EGL
  egl_dpy = eglGetDisplay (nat_dpy);
  if (egl_dpy == EGL_NO_DISPLAY) {
    error ("could not get and EGL display");
    return false;
  }

  int egl_maj, egl_min;
  if (! eglInitialize (egl_dpy, &egl_maj, &egl_min)) {
    error ("could not initialise the EGL display");
    return 1;
  }
  cout << "EGL version " << egl_maj << '.' << egl_min << endl;

  // Create a system window
  int ret = make_native_window (nat_dpy, egl_dpy,
				win_width, win_height, fullscreen,
				win_title.c_str (),
				&nat_win, &config);
  if (ret == 0)
    return false;

  eglBindAPI(EGL_OPENGL_ES_API);

  // Check for swap intervals
  EGLint si;
  if (! eglGetConfigAttrib (egl_dpy, config,
			    EGL_MIN_SWAP_INTERVAL, &si)) {
    fprintf (stderr, "could not get minimal swap interval\n");
    return false;
  }
  cout << "Minimal swap interval: " << si << endl;
  if (! eglGetConfigAttrib (egl_dpy, config,
			    EGL_MAX_SWAP_INTERVAL, &si)) {
    fprintf (stderr, "could not get maximal swap interval\n");
    return false;
  }
  cout << "Maximal swap interval: " << si << endl;


  // Create an OpenGL ES 2 context
  static const EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  ctx = eglCreateContext (egl_dpy, config,
			  EGL_NO_CONTEXT, ctx_attribs);
  if (ctx == EGL_NO_CONTEXT) {
    error ("could not create an EGL context");
    return false;
  }
  {
    EGLint val;
    eglQueryContext (egl_dpy, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
    if (val != 2) {
      error ("EGL v2 required");
      return false;
    }
  }

  // Create the EGL surface associated with the window
  sur = eglCreateWindowSurface (egl_dpy, config,
				nat_win, NULL);
  if (sur == EGL_NO_SURFACE) {
    error ("could not create an EGL surface");
    return false;
  }

  // Set the current EGL context
  eglMakeCurrent (egl_dpy, sur, sur, ctx);

  // Display OpenGL ES information
  printf ("OpenGL ES version  %s by %s (%s) with: %s\n",
	  glGetString (GL_VERSION),
	  glGetString (GL_VENDOR),
	  glGetString (GL_RENDERER),
	  glGetString (GL_EXTENSIONS));

  glClearColor (0, 0, 0, 0);

  // Frames as textures
  printf ("Creating %d texture frames\n", nframes);
  tframes = new GLuint[nframes];
  if (tframes == NULL) {
    error ("could not allocate memory for the texture identifiers");
    return false;
  }
  glGenTextures (nframes, tframes);
  assert_gl_error ("generate textures");
  
  // Create a fragment shader for the texture
  stringstream ss;
  static const char *fshader_txt = 
    "varying mediump vec2 tex_coord;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    //"  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "  gl_FragColor = texture2D(texture, tex_coord);\n"
    //"  gl_FragColor = vec4(tex_coord.x, tex_coord.y, 0, 1);\n"
    "}\n";
  GLuint fshader = glCreateShader (GL_FRAGMENT_SHADER);
  assert_gl_error ("create a fragment shader");
  if (fshader == 0) {
    fprintf (stderr, "could not create a fragment shader (0x%x)\n",
	     glGetError ());
    return 1;
  }
  glShaderSource (fshader, 1, (const char **) &fshader_txt, NULL);
  glCompileShader (fshader);
  GLint stat;
  glGetShaderiv (fshader, GL_COMPILE_STATUS, &stat);
  if (stat == 0) {
    GLsizei len;
    char log[1024];
    glGetShaderInfoLog (fshader, 1024, &len, log);
    cerr << "could not compile the fragment shader:" << endl
         << log << endl;
    return 1;
  }

  // Compute the offset to center the stimulus
  GLfloat offx = (win_width-tex_width)/2.0f;
  GLfloat offy = (win_height-tex_height)/2.0f;

  float tx_conv = tex_width / (2.0f * win_width);
  float ty_conv = tex_height / (2.0f * win_height);

  tx_conv = 1/2.0f;
  ty_conv = 1/2.0f;

  // Create an identity vertex shader
  ss.str("");
  ss << fixed << setprecision(12)
     // This projection matrix maps OpenGL coordinates
     // to device coordinates (pixels)
     << "const mat4 proj_matrix = mat4("
     << (2.0/win_width) << ", 0.0, 0.0, -1.0," << endl
     << "0.0, " << -(2.0/win_height) << ", 0.0, 1.0," << endl
     << "0.0, 0.0, -1.0, 0.0," << endl
     << "0.0, 0.0, 0.0, 1.0);" << endl
     << "attribute vec2 ppos;" << endl
     << "varying vec2 tex_coord;" << endl
     << "void main () {" << endl
     << "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0) * proj_matrix;" << endl
     << "  tex_coord = vec2((ppos.x-" << offx << ")/" << (float) tex_width 
     << ", (ppos.y-" << offy << ")/" << (float) tex_height << " );" << endl
     << "}" << endl;
  auto vshader_str = ss.str();
  cout << "vertex shader:" << endl << vshader_str << endl;

  GLuint vshader = glCreateShader (GL_VERTEX_SHADER);
  if (vshader == 0) {
    fprintf (stderr, "could not create a vertex shader (0x%x)\n",
	     glGetError ());
    return 1;
  }
  assert_gl_error ("create a vertex shader");

  const char* vshader_txt = vshader_str.c_str();
  glShaderSource (vshader, 1, (const char **) &vshader_txt, NULL);
  glCompileShader (vshader);
  glGetShaderiv (vshader, GL_COMPILE_STATUS, &stat);
  if (stat == 0) {
    GLsizei len;
    char log[1024];
    glGetShaderInfoLog (vshader, 1024, &len, log);
    cerr << "could not compile the vertex shader:" << endl
         << log << endl;
    return 1;
  }

  // Add the fragment shader to the current program
  GLuint program = glCreateProgram ();
  glAttachShader (program, fshader);
  glAttachShader (program, vshader);
  glLinkProgram (program);
  glGetProgramiv (program, GL_LINK_STATUS, &stat);
  if (stat == 0) {
    GLsizei len;
    char log[1024];
    glGetProgramInfoLog (program, 1024, &len, log);
    fprintf (stderr,
	     "could not link the shaders:\n%s\n", log);
    return 1;
  }
  glUseProgram (program);
  assert_gl_error ("use the shaders program");

  int ppos = glGetAttribLocation (program, "ppos");
  if (ppos == -1) {
    fprintf (stderr, "could not get attribute ‘ppos’\n");
    return 1;
  }

  // Rectangle covering the full texture
  static GLfloat vertices[] = {
    offx, offy,
    offx, offy + tex_width,
    offx + tex_width, offy + tex_height,

    offx + tex_width, offy + tex_height,
    offx, offy,
    offx + tex_width, offy
  };
  glEnableVertexAttribArray (ppos);
  glVertexAttribPointer (ppos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glClear (GL_COLOR_BUFFER_BIT);

  glActiveTexture (GL_TEXTURE0);

  texloc = glGetUniformLocation (program, "texture");
  if (texloc == -1) {
    cerr << "could not get location of ‘texture’" << endl;
    return 1;
  }
  assert_gl_error ("get location of uniform ‘texture’");

  
  return true;
}

void
Experiment::egl_cleanup ()
{
  delete [] tframes;

  eglDestroyContext (egl_dpy, ctx);
  eglDestroySurface (egl_dpy, sur);
  eglTerminate (egl_dpy);

  Display *dpy = (Display*) nat_dpy;
  XDestroyWindow (dpy, (Window) nat_win);
  XCloseDisplay (dpy);
}

void
Experiment::error (const std::string& msg)
{
  cerr << "error: " << msg << endl;
}

Experiment::Experiment ()
  : swap_interval (1)
{
}

Experiment::~Experiment ()
{
  egl_cleanup ();
}
