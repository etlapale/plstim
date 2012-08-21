#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <X11/Xlib.h>
# include <X11/Xatom.h>
#endif

#include <EGL/egl.h>

#include <GLES2/gl2.h>


// If possible, use a clock not sensible to adjtime or ntp
// (Linux only)
#define CLOCK CLOCK_MONOTONIC_RAW

// Otherwise switch back to a clock at least not sensible to
// administrator clock settings
//#define CLOCK CLOCK_MONOTONIC


static EGLNativeDisplayType
open_native_display (void* param)
{
#ifdef _WIN32
  return (EGLNativeDisplayType) 0;
#else
  return (EGLNativeDisplayType) XOpenDisplay ((char*) param);
#endif
}

static int
make_native_window (EGLNativeDisplayType nat_dpy, EGLDisplay egl_dpy,
		    int width, int height, const char* title,
		    EGLNativeWindowType *window, EGLConfig *conf)
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
    return 0;
  }

#ifdef _WIN32
#else
  // Get the default screen and root window
  Display *dpy = (Display*) nat_dpy;
  Window root = DefaultRootWindow (dpy);
  Window scrnum = DefaultScreen (dpy);
#endif

  // Fetch visual information for the configuration
  EGLint vid;
  int num_visuals;
  if (! eglGetConfigAttrib (egl_dpy, config,
			    EGL_NATIVE_VISUAL_ID, &vid)) {
    fprintf (stderr, "could not get native visual id\n");
    return 0;
  }
#ifdef _WIN32
#else
  XVisualInfo xvis;
  XVisualInfo *xvinfo;
  xvis.visualid = vid;
  xvinfo = XGetVisualInfo (dpy, VisualIDMask, &xvis, &num_visuals);
  if (num_visuals == 0) {
    fprintf (stderr, "could not get the X visuals\n");
    return 0;
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
  win = XCreateWindow (dpy, root, 0, 0, width, height, 0,
			xvinfo->depth, InputOutput, xvinfo->visual,
			mask, &xattr);
  *window = (EGLNativeWindowType) win;

  // Set X hints and properties
  XStoreName (dpy, win, title);

  XMapWindow (dpy, win);
  XFree (xvinfo);
#endif

  // Make the window fullscreen
#if 0
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
#endif

  *conf = config;
  return 1;
}


int
main (int argc, char* argv[])
{
  int res;

  EGLNativeDisplayType nat_dpy;
  EGLNativeWindowType nat_win;

  EGLDisplay egl_dpy;
  EGLConfig config;
  EGLint egl_maj, egl_min;

  int i;
  int nframes = 60*1;
  unsigned int width = 300, height = 300;

#ifdef _WIN32
#else
  struct timespec tp_start, tp_stop;

  // Get the clock resolution
  res = clock_getres (CLOCK, &tp_start);
  if (res < 0) {
    perror ("clock_getres");
    return 1;
  }
  printf ("clock %d resolution: %lds %ldns\n",
	  CLOCK, tp_start.tv_sec, tp_start.tv_nsec);
#endif

  // Setup X11
  nat_dpy = open_native_display (NULL);
#if 0
  if (dpy == NULL) {
    fprintf (stderr, "could not open X display\n");
    return 1;
  }
#endif

  // Setup EGL
  egl_dpy = eglGetDisplay (nat_dpy);
  if (egl_dpy == EGL_NO_DISPLAY) {
    fprintf (stderr, "could not get an EGL display\n");
    return 1;
  }
  if (! eglInitialize (egl_dpy, &egl_maj, &egl_min)) {
    fprintf (stderr, "could not init the EGL display\n");
    return 1;
  }
  printf ("EGL version %d.%d\n", egl_maj, egl_min);

  // Create a system window
  int ret = make_native_window (nat_dpy, egl_dpy,
				width, height, argv[0],
				&nat_win, &config);
  if (ret == 0)
    return 1;

  eglBindAPI(EGL_OPENGL_ES_API);

  // Create an OpenGL ES 2 context
  static const EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  EGLContext ctx = eglCreateContext (egl_dpy, config,
				     EGL_NO_CONTEXT, ctx_attribs);
  if (ctx == EGL_NO_CONTEXT) {
    fprintf (stderr, "could not create an EGL context\n");
    return 1;
  }
  {
    EGLint val;
    eglQueryContext(egl_dpy, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
    assert(val == 2);
  }

  // Create the EGL surface associated with the window
  EGLSurface sur = eglCreateWindowSurface (egl_dpy, config,
					   nat_win, NULL);
  if (sur == EGL_NO_SURFACE) {
    fprintf (stderr, "could not create an EGL surface\n");
    return 1;
  }

  // Set the current EGL context
  eglMakeCurrent (egl_dpy, sur, sur, ctx);

  // Display OpenGL ES information
  printf ("OpenGL ES version  %s by %s (%s) with: %s\n",
	  glGetString (GL_VERSION),
	  glGetString (GL_VENDOR),
	  glGetString (GL_RENDERER),
	  glGetString (GL_EXTENSIONS));

  glClearColor (0.1, 0.1, 0.1, 1.0);
  glViewport (0, 0, width, height);

  //GLfloat vertices[] = {0,0, 100,0, 0,100};
  GLfloat vertices[] = {
    0,0.5,
    -0.5,-0.5,
    0.5,-0.5
  };

  // Create a fragment shader for uniform colour
  static const char *fshader_txt = 
    "void main() {\n"
    "  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\n";
  GLuint fshader = glCreateShader (GL_FRAGMENT_SHADER);
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
    fprintf (stderr, "could not compile the fragment shader\n");
    return 1;
  }

  // Create an identity vertex shader
  static const char *vshader_txt =
    "attribute vec2 ppos;\n"
    "void main() {\n"
    "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0);\n"
    "}\n";
  GLuint vshader = glCreateShader (GL_VERTEX_SHADER);
  if (vshader == 0) {
    fprintf (stderr, "could not create a vertex shader (0x%x)\n",
	     glGetError ());
    return 1;
  }
  glShaderSource (vshader, 1, (const char **) &vshader_txt, NULL);
  glCompileShader (vshader);
  glGetShaderiv (vshader, GL_COMPILE_STATUS, &stat);
  if (stat == 0) {
    fprintf (stderr, "could not compile the vertex shader\n");
    return 1;
  }

  // Add the fragment shader to the current program
  GLuint program = glCreateProgram ();
  glAttachShader (program, vshader);
  glAttachShader (program, fshader);
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

  glValidateProgram (program);
  glUseProgram (program);

  int ppos = glGetAttribLocation (program, "ppos");
  if (ppos == -1) {
    fprintf (stderr, "could not get attribute ‘ppos’\n");
    return 1;
  }

#ifdef _WIN32
#else
  // Mark the beginning time
  res = clock_gettime (CLOCK, &tp_start);
#endif

  glEnableVertexAttribArray (ppos);
  glVertexAttribPointer (ppos, 2, GL_FLOAT, GL_FALSE, 0, vertices);

  for (i = 0; i < nframes; i++) {
    glClear (GL_COLOR_BUFFER_BIT);
    glDrawArrays (GL_TRIANGLES, 0, 3);


    eglSwapBuffers (egl_dpy, sur);
  }

#ifdef _WIN32
#else
  // Mark the end time
  res = clock_gettime (CLOCK, &tp_stop);

  // Display the elapsed time
  printf ("start: %lds %ldns\n",
	  tp_start.tv_sec, tp_start.tv_nsec);
  printf ("stop:  %lds %ldns\n",
	  tp_stop.tv_sec, tp_stop.tv_nsec);
#endif

  // Cleanup
  eglDestroyContext (egl_dpy, ctx);
  eglDestroySurface (egl_dpy, sur);
  eglTerminate (egl_dpy);

#ifdef _WIN32
#else
  Display *dpy = (Display*) nat_dpy;
  XDestroyWindow (dpy, (Window) nat_win);
  XCloseDisplay (dpy);
#endif

  return 0;
}
// vim:sw=2:
