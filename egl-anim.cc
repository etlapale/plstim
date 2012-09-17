#include <assert.h>
#include <errno.h>
#include <setjmp.h>
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


#include <png.h>


#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;


// If possible, use a clock not sensible to adjtime or ntp
// (Linux only)
#define CLOCK CLOCK_MONOTONIC_RAW

// Otherwise switch back to a clock at least not sensible to
// administrator clock settings
//#define CLOCK CLOCK_MONOTONIC


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


static EGLNativeDisplayType
open_native_display (void* param)
{
#ifdef _WIN32
  return (EGLNativeDisplayType) 0;
#else
  return (EGLNativeDisplayType) XOpenDisplay ((char*) param);
#endif
}

static bool
load_png_image (const char* path, int* width, int* height,
		GLint* tex_type, GLubyte** data)
{
  // Open the input file
  FILE* fp = fopen (path, "rb");
  if (! fp) {
    fprintf (stderr, "could not open the input frame ‘%s’: %s\n",
	     path, strerror (errno));
    return false;
  }

  // Make sure it’s a PNG
  png_byte magic[8];
  fread (magic, 1, sizeof (magic), fp);
  if (! png_check_sig (magic, sizeof (magic))) {
    fprintf (stderr, "‘%s’ is not a valid PNG\n", path);
    fclose (fp);
    return false;
  }

  // Initialise PNG structures
  auto png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
					 NULL, NULL, NULL);
  if (! png_ptr) {
    fprintf (stderr, "could not initialise libpng reader\n");
    fclose (fp);
    return false;
  }
  auto info_ptr = png_create_info_struct (png_ptr);
  if (! info_ptr) {
    fprintf (stderr, "could not initialise libpng info\n");
    png_destroy_read_struct (&png_ptr, NULL, NULL);
    fclose (fp);
    return false;
  }

  // Declare allocated arrays to delete them on error
  GLubyte* pixels = NULL;
  png_bytep* row_pointers = NULL;

  // Error handling
  if (setjmp (png_jmpbuf (png_ptr))) {
    fprintf (stderr, "error while processing the PNG\n");
    if (pixels != NULL)
      delete [] pixels;
    if (row_pointers != NULL)
      delete [] row_pointers;
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    fclose (fp);
    return false;
  }

  // Start reading the PNG
  png_init_io (png_ptr, fp);
  png_set_sig_bytes (png_ptr, sizeof (magic));
  png_read_info (png_ptr, info_ptr);

  // Check info
  int depth = png_get_bit_depth (png_ptr, info_ptr);
  int type = png_get_color_type (png_ptr, info_ptr);
  printf ("  Image depth: %d, type: %d\n", depth, type);
  if (type == PNG_COLOR_MASK_PALETTE)
    png_set_palette_to_rgb (png_ptr);
  /*if (type == PNG_COLOR_TYPE_GRAY && depth < 8)
    png_set_gray_1*/
  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha (png_ptr);
  png_read_update_info (png_ptr, info_ptr);

  // Read image info
  png_uint_32 pwidth, pheight;
  png_get_IHDR (png_ptr, info_ptr,
		(png_uint_32*) &pwidth,
		(png_uint_32*) &pheight,
		&depth, &type,
		NULL, NULL, NULL);
  int bpp;
  GLint gltype;
  switch (type) {
  case PNG_COLOR_TYPE_GRAY:
    bpp = 1;
    gltype = GL_LUMINANCE;
    break;
  case PNG_COLOR_TYPE_RGB:
    bpp = 3;
    gltype = GL_RGB;
    break;
  case PNG_COLOR_TYPE_RGBA:
    bpp = 4;
    gltype = GL_RGBA;
    break;
  default:
    fprintf (stderr, "unknown color type\n");
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
    return false;
  }

  // Read the pixels
  pixels = new GLubyte[pwidth*pheight*bpp];
  row_pointers = new png_bytep[pheight];
  for (png_uint_32 i = 0; i < pheight; i++)
    row_pointers[i] = pixels + i*pwidth*bpp;
  png_read_image (png_ptr, row_pointers);

  // Cleanup
  delete [] row_pointers;
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
  fclose (fp);

  // Store the results
  *width = pwidth;
  *height = pheight;
  *tex_type = gltype;
  *data = pixels;

  return true;
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
  unsigned int width = 512, height = 512;

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

  //glClearColor (0.1, 0.1, 0.1, 1.0);
  glClearColor (1, 1, 1, 1);
  //glViewport (0, 0, width, height);

  // Load the frames as textures
  int nframes = argc - 1;
  printf ("Creating %d texture frames\n", nframes);
  auto tframes = new GLuint[nframes];
  glGenTextures (nframes, tframes);
  assert_gl_error ("generate textures");
  // ...
  for (int i = 0; i < nframes; i++) {
    // Load the image as a PNG file
    GLubyte* data;
    int twidth, theight;
    GLint gltype;
    if (! load_png_image (argv[i+1], &twidth, &theight,
			  &gltype, &data))
      return 1;

    glBindTexture (GL_TEXTURE_2D, tframes[i]);
    assert_gl_error ("bind a texture");
    cout << "texture size: " << twidth << "×" << theight << endl
         << "texture type: " << hex << gltype << dec << endl;
    glTexImage2D (GL_TEXTURE_2D, 0, gltype, twidth, theight,
		  0, gltype, GL_UNSIGNED_BYTE, data);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    assert_gl_error ("set texture data");

    // Erase the memory, OpenGL has its own copy
    delete [] data;
  }
  cout << nframes << " frames loaded" << endl;


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

  // Create an identity vertex shader
  ss.str("");
  ss << fixed << setprecision(12)
     // This projection matrix maps OpenGL coordinates
     // to device coordinates (pixels)
     << "const mat4 proj_matrix = mat4("
     << (2.0/width) << ", 0.0, 0.0, -1.0," << endl
     << "0.0, " << -(2.0/height) << ", 0.0, 1.0," << endl
     << "0.0, 0.0, -1.0, 0.0," << endl
     << "0.0, 0.0, 0.0, 1.0);" << endl
     << "attribute vec2 ppos;" << endl
     << "varying vec2 tex_coord;" << endl
     << "void main () {" << endl
     << "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0) * proj_matrix;" << endl
     //<< "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0);" << endl
     //<< "  tex_coord = vec2(ppos.x, ppos.y);" << endl
     << "  tex_coord = vec2((gl_Position.x+1.0)/2.0, (-gl_Position.y+1.0)/2.0);" << endl
     << "}" << endl;
  auto vshader_str = ss.str();
#if 0
  static const char *vshader_txt =
    "attribute vec2 ppos;\n"
    "void main() {\n"
    "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0);\n"
    "}\n";
#endif
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

  // Create the vertex buffer
#if 0
  static GLfloat vertices[] = {
    -1, -1,
    -1,  1,
     1,  1,

     1,  1,
    -1, -1,
     1, -1
  };
#else
  static GLfloat vertices[] = {
#if 1
#if 1
    0, 0,
    0, (GLfloat) height,
    (GLfloat) width, (GLfloat) height,

    (GLfloat) width, (GLfloat) height,
    0, 0,
    (GLfloat) width, 0
#else
    10.0f, 10.0f,
    10.0f, (GLfloat) height - 10.0f,
    (GLfloat) width - 10.0f, (GLfloat) height - 10.0f,

    (GLfloat) width - 10.0f, (GLfloat) height - 10.0f,
    10.0f, 10.0f,
    (GLfloat) width - 10.0f, 10.0f
#endif
#else
    4, 4,
    100, 4,
    4, 12
#endif
  };
#endif
  /*GLuint vbuf;
  glGenBuffers (1, &vbuf);
  glBindBuffer (GL_ARRAY_BUFFER, vbuf);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);
*/
  glEnableVertexAttribArray (ppos);
  glVertexAttribPointer (ppos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glClear (GL_COLOR_BUFFER_BIT);

  glActiveTexture (GL_TEXTURE0);

  int texloc = glGetUniformLocation (program, "texture");
  if (texloc == -1) {
    cerr << "could not get location of ‘texture’" << endl;
    return 1;
  }
  assert_gl_error ("get location of uniform ‘texture’");

  cout << "Starting" << endl;
#ifdef _WIN32
#else
  // Mark the beginning time
  res = clock_gettime (CLOCK, &tp_start);
#endif


  for (i = 0; i < nframes; i++) {
    glBindTexture (GL_TEXTURE_2D, tframes[i%nframes]);
    glUniform1i (texloc, 0);

    glDrawArrays (GL_TRIANGLES, 0, 6);
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
