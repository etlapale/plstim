// lorenceau-experiment.cc – Experiment of Lorenceau et al (1993)

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <unistd.h>
#include <sys/time.h>


#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

// Command line parsing
#include <tclap/CmdLine.h>

// Graphics library
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
using namespace Cairo;


// Stimulus library
#include <plstim/plstim.h>
using namespace plstim;


// If possible, use a clock not sensible to adjtime or ntp
// (Linux only)
#define CLOCK CLOCK_MONOTONIC_RAW

// Otherwise switch back to a clock at least not sensible to
// administrator clock settings
//#define CLOCK CLOCK_MONOTONIC


class LorenceauExperiment : public Experiment
{
protected:
  std::vector<int> answer_keys;
public:
  LorenceauExperiment ();
  virtual bool run_trial ();
};


LorenceauExperiment::LorenceauExperiment ()
{
  ntrials = 400;
  answer_keys.push_back (XK_Up);
  answer_keys.push_back (XK_Down);
}


bool
LorenceauExperiment::run_trial ()
{
  struct timespec tp_start, tp_stop;

  // Wait for a key press before running the trial
  if (! wait_any_key ())
    return false;

  //cout << "Starting" << endl;
  // Mark the beginning time
  int res = clock_gettime (CLOCK, &tp_start);

  // Display each frame of the trial
  if (! show_frames ())
    return false;

  // Mark the end time
  res = clock_gettime (CLOCK, &tp_stop);

  // Display the elapsed time
  printf ("start: %lds %ldns\n",
	  tp_start.tv_sec, tp_start.tv_nsec);
  printf ("stop:  %lds %ldns\n",
	  tp_stop.tv_sec, tp_stop.tv_nsec);

  // Wait for a key press at the end of the trial
  if (! clear_screen ())
    return false;
  int pressed_key;
  if (! wait_for_key (answer_keys, &pressed_key))
    return false;

  cout << "Answer: " << (pressed_key == XK_Up ? "up" : "down") << endl;
  return true;
}


int
main (int argc, char* argv[])
{
  int res;

  EGLint egl_maj, egl_min;

  int i;
  unsigned int width, height;

  std::vector<std::string> images;
  EGLint swap_interval;
  
  // Physical setup configuration
  Setup setup;
  bool fullscreen = false;

  // Experiment
  LorenceauExperiment xp;

  try {
    // Setup the command line parser
    TCLAP::CmdLine cmd ("Generate, display and record stimuli",
			' ', "0.1");
    TCLAP::ValueArg<string> arg_geom ("g", "geometry",
	"Stimulus dimension in pixels", false, "", "WIDTHxHEIGHT");
    cmd.add (arg_geom);
    TCLAP::ValueArg<int> arg_swap ("", "swap-interval",
	"Swap interval in frames", false, 1, "INTERVAL");
    cmd.add (arg_swap);
    TCLAP::ValueArg<std::string> setup_arg ("s", "setup",
	"Hardware setup configuration", false, Setup::default_source, "SETUP");
    cmd.add (setup_arg);
    // Parser the command line arguments
    cmd.parse (argc, argv);

    // Store the setup configuration
    if (! setup.load (setup_arg.getValue ()))
      return 1;

    // Parse the geometry
    auto geom = arg_geom.getValue ();
    if (geom.empty ()) {
      width = setup.resolution[0];
      height = setup.resolution[1];
      fullscreen = true;
    }
    else {
      size_t sep = geom.find ('x');
      if (sep == geom.npos) {
	cerr << "error: invalid stimulus dimension: " << geom << endl;
	return 1;
      }
      width = atof (geom.c_str ());
      height = atof (geom.c_str () + sep + 1);
      if (width == 0 || height == 0) {
	cerr << "error: invalid stimulus dimension: " << geom << endl;
	return 1;
      }
    }

    swap_interval = arg_swap.getValue ();

  } catch (TCLAP::ArgException& e) {
    cerr << "error: could not parse argument "
         << e.argId () << ": " << e.error () << endl;
  }
  
  // Debug the configuration out
  cout << "Refresh rate: " << setup.refresh << " Hz" << endl
       << "Viewing distance: " << setup.distance << " mm" << endl
       << "Resolution: " << setup.resolution[0] << "×" << setup.resolution[1] << " px" << endl
       << "Size: " << setup.size[0] << "×" << setup.size[1] << " mm" << endl
       << "Luminance: " << setup.luminance[0] << "–" << setup.luminance[1] << " cd/m²" << endl;
  // Debug calculus
  cout << "Average resolution: " << setup.px_mm << " px/mm" << endl;
  cout << "Visual field: " << setup.pix2deg (setup.resolution[0])
       << "×" << setup.pix2deg (setup.resolution[1])
       << " degrees" << endl
       << endl;

  struct timespec tp_start, tp_stop;

  // Get the clock resolution
  res = clock_getres (CLOCK, &tp_start);
  if (res < 0) {
    perror ("clock_getres");
    return 1;
  }
  printf ("clock %d resolution: %lds %ldns\n",
	  CLOCK, tp_start.tv_sec, tp_start.tv_nsec);

#if 0
  // Load the frames as textures
  int prev_twidth = -1, prev_theight;
  int twidth, theight;
  // ...
  for (int i = 0; i < nframes; i++) {
    // Load the image as a PNG file
    GLubyte* data;
    GLint gltype;
    if (! load_png_image (images[i].c_str (), &twidth, &theight,
			  &gltype, &data))
      return 1;

    // Make sure the texture sizes match
    if (prev_twidth >= 0
	&& (prev_twidth != twidth || prev_theight != theight)) {
      fprintf (stderr, "error: texture sizes do not match\n");
      return 1;
    }
    prev_twidth = twidth;
    prev_theight = theight;

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
#endif

  int twidth, theight;
  twidth = theight = 1024;

  // Initialise the graphic system
  xp.egl_init (width, height, fullscreen, argv[0],
	       twidth, theight);


  // Run the session
  if (! xp.run_session ())
    return 1;


  // Cleanup
  xp.egl_cleanup ();

  return 0;
}
// vim:sw=2:
