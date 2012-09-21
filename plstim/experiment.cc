#include <X11/Xlib.h>
#include <X11/Xatom.h>

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
		    int width, int height, bool fullscreen,
		    const char* title,
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

  // Get the default screen and root window
  Display *dpy = (Display*) nat_dpy;
  Window root = DefaultRootWindow (dpy);
  Window scrnum = DefaultScreen (dpy);

  // Fetch visual information for the configuration
  EGLint vid;
  int num_visuals;
  if (! eglGetConfigAttrib (egl_dpy, config,
			    EGL_NATIVE_VISUAL_ID, &vid)) {
    fprintf (stderr, "could not get native visual id\n");
    return 0;
  }
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
  return 1;
}

bool
Experiment::run_session ()
{
  for (int i = 0; i < ntrials; i++)
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
Experiment::wait_for_key (const std::vector<int>& accepted_keys,
			  int* pressed_key)
{
  bool answer_up;
  KeySym keysym;
  XComposeStatus compose;
  const int buflen = 10;
  char buf[buflen];
  XEvent evt;
  for (;;) {
    XNextEvent ((Display*) nat_dpy, &evt);
    if (evt.type == KeyPress) {
      int count = XLookupString (&evt.xkey, buf, buflen,
				 &keysym, &compose);
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
Experiment::show_frames ()
{
  for (int i = 0; i < nframes; i++) {
    // Set the current frame
    glBindTexture (GL_TEXTURE_2D, tframes[i%nframes]);
    glUniform1i (texloc, 0);

    // Draw the frame with triangles
    glDrawArrays (GL_TRIANGLES, 0, 6);
    
    // Set the swap interval
    if (swap_interval != 1)
      eglSwapInterval (egl_dpy, swap_interval);

    // Swap
    eglSwapBuffers (egl_dpy, sur);
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
Experiment::egl_init (int width, int height, bool fullscreen,
		      const std::string& window_title,
		      int texture_width, int texture_height)
{
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
				width, height, fullscreen,
				window_title.c_str (),
				&nat_win, &config);
  if (ret == 0)
    return false;

  eglBindAPI(EGL_OPENGL_ES_API);

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
    eglQueryContext(egl_dpy, ctx, EGL_CONTEXT_CLIENT_VERSION, &val);
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

  //glClearColor (0.1, 0.1, 0.1, 1.0);
  glClearColor (0, 0, 0, 0);
  //glViewport (0, 0, width, height);


  // Frames as textures
  printf ("Creating %d texture frames\n", nframes);
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

  // Offset to center the stimulus
  GLfloat offx = (width-texture_width)/2.0f;
  GLfloat offy = (height-texture_height)/2.0f;

  float tx_conv = texture_width / (2.0f * width);
  float ty_conv = texture_height / (2.0f * height);

  tx_conv = 1/2.0f;
  ty_conv = 1/2.0f;

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
     << "  tex_coord = vec2((ppos.x-" << offx << ")/" << (float) texture_width 
     << ", (ppos.y-" << offy << ")/" << (float) texture_height << " );" << endl
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

  static GLfloat vertices[] = {
    offx, offy,
    offx, offy + texture_height,
    offx + texture_width, offy + texture_height,

    offx + texture_width, offy + texture_height,
    offx, offy,
    offx + texture_width, offy
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
