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
  /// Associated hardware setup
  Setup* setup;

  std::vector<int> answer_keys;

  /// Foreground colour
  float fg;
  /// Background colour
  float bg;
  /// Line length
  float ll;
  /// Line width
  float lw;

  float sx;
  float sy;
  float spacing;
  int bx;
  int by;

  /// Aperture radius
  float radius;

  /// Orientation of the lines
  bool cw;

  /// Real motion direction
  bool up;

  /// Control or test experiment
  bool control;

public:
  LorenceauExperiment (Setup* s);
  virtual bool run_trial ();
  virtual bool make_frames ();
};


LorenceauExperiment::LorenceauExperiment (Setup* s)
  : setup (s)
{
  ntrials = 400;
  answer_keys.push_back (XK_Up);
  answer_keys.push_back (XK_Down);

  // Stimulus configuration
  float ll_deg = 2.7;
  ll = setup->deg2pix (ll_deg);
  cout << "Line length is " << ll_deg << " degrees, or "
       << ll << " pixels" << endl;
  float lw_deg = sec2deg (1.02);
  lw = setup->deg2pix (lw_deg);
  cout << "Line width is " << lw_deg << " degrees, or "
       << lw << " pixels" << endl;
  float bg_cd = 20;
  bg = setup->lum2px (bg_cd);
  cout << "Background luminance is " << bg_cd << " cd/m², or "
       << bg << " in [0,1]" << endl;
  // C%→L: C₁₂→13.44, C₂₅→15, C₃₉→16.68, C₅₂→18.24, C₇₀→20.4
  float fg_cd = 150;//16.68;
  fg = setup->lum2px (fg_cd);
  cout << "Foreground luminance is " << fg_cd << " cd/m², or "
       << fg << " in [0,1]" << endl;

  // Line geometry
  float orient = radians (cw ? 110 : 70);
  bx = (int) (ll*cos(orient));
  by = (int) (ll*sin(orient));
  sx = abs (bx);
  sy = abs (by);

  // Set the bar positions
  spacing = setup->deg2pix (1);
}


bool
LorenceauExperiment::run_trial ()
{
  struct timespec tp_start, tp_stop;

  make_frames ();

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

bool
LorenceauExperiment::make_frames ()
{
  // Create a Cairo context to draw the frames
  auto sur = ImageSurface::create (Cairo::FORMAT_RGB24,
				   twidth, theight);
  auto cr = Cairo::Context::create (sur);

  radius = (twidth < theight ? twidth / 2 : theight / 2) - 1;

  // Make sure texture dimensions are power of 2
  int lgw = 1 << ((int) log2f (twidth));
  int lgh = 1 << ((int) log2f (theight));
  if (lgw < twidth)
    lgw <<= 1;
  if (lgh < theight)
    lgh <<= 1;
  cout << "Power of 2 dimensions: " << lgw << "×" << lgh << endl;
  int offx = (lgw-twidth)/2;
  int offy = (lgh-theight)/2;

  // Line speed and direction
  float orient = radians (cw ? 110 : 70);
  float direction = fmod (orient
    + (orient < M_PI/2 ? -1 : +1) * (M_PI/2)	// Orientation
    + (up ? 0 : 1) * M_PI			// Up/down
    + (orient < M_PI/2 ? -1 : 1) * (control ? 0 : 1) * radians (140)
    , 2*M_PI);
  cout << "Line direction: " << degrees (direction) << "°" << endl;
  float speed = setup->ds2pf (6.5);
  int dx = (int) (speed*cos (direction));
  int dy = (int) (speed*sin (direction));
  cout << "Line speed is 6.5 deg/s, or "
    << speed << " px/frame ("
    << (dx >= 0 ? "+" : "") << dx
    << (dy >= 0 ? "+" : "") << dy
    << ")" << endl;

  // Render each frame
  cr->set_antialias (Cairo::ANTIALIAS_NONE);
  cr->set_fill_rule (Cairo::FILL_RULE_EVEN_ODD);
  for (int i = 0; i < nframes; i++) {

    // Background
    cr->set_source_rgb (bg, bg, bg);
    cr->paint ();

    // Lines
    cr->set_line_width (lw);
    cr->set_source_rgb (fg, fg, fg);
    for (int x = offx+i*dx - (sx+spacing); x < twidth+sx+spacing; x+=sx+spacing) {
      for (int y = offy+i*dy; y < theight+sy+spacing; y+=sy+spacing) {
	cr->move_to (x, y);
	cr->rel_line_to (bx, by);
      }
    }
    cr->stroke ();

    // Aperture
    cr->translate (-offx, -offy);
    cr->rectangle (0, 0, lgw, lgh);
    cr->arc (lgw/2, lgh/2, radius, 0, 2*M_PI);
    cr->set_source_rgb (1, 0, 0);
    cr->fill ();
    cr->translate (offx, offy);

    // Save the frame as PNG
    //sur->write_to_png ("output.png");

    // Create an OpenGL texture for the frame
    glBindTexture (GL_TEXTURE_2D, tframes[i]);
    if (glGetError () != GL_NO_ERROR) {
      cerr << "error: could not bind a texture" << endl;
      return false;
    }
    GLint gltype = GL_RGBA;
    glTexImage2D (GL_TEXTURE_2D, 0, gltype, twidth, theight,
		  0, gltype, GL_UNSIGNED_BYTE, sur->get_data ());
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (glGetError () != GL_NO_ERROR) {
      cerr << "error: could not set texture data" << endl;
      return false;
    }
  }

  cout << nframes << " frames loaded" << endl;

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
  
  // Physical setup configuration
  Setup setup;
  bool fullscreen = false;

  try {
    // Setup the command line parser
    TCLAP::CmdLine cmd ("Generate, display and record stimuli",
			' ', "0.1");
    TCLAP::ValueArg<string> arg_geom ("g", "geometry",
	"Stimulus dimension in pixels", false, "", "WIDTHxHEIGHT");
    cmd.add (arg_geom);
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
       << " degrees" << endl;

  // Experiment
  LorenceauExperiment xp (&setup);

  // Get a swap interval for the desired frequency
  float wanted_frequency = 30;	// Hz
  int coef = (int) nearbyintf (setup.refresh / wanted_frequency);
  if ((setup.refresh/coef - wanted_frequency)/wanted_frequency > 0.01) {
    cerr << "error: cannot set monitor frequency at 1% of the desired frequency" << endl;
    return 1;
  }
  float dur_ms = 332;
  int nframes = (int) nearbyintf ((setup.refresh/coef)*(dur_ms/1000.));
  cout << "Selected refresh rate: " << (setup.refresh/coef) << " Hz" << endl;
  cout << "Duration: " << dur_ms << " ms, or " << nframes << " frames" << endl;


  // Get the clock resolution
  struct timespec tp;
  res = clock_getres (CLOCK, &tp);
  if (res < 0) {
    perror ("clock_getres");
    return 1;
  }
  printf ("clock %d resolution: %lds %ldns\n",
	  CLOCK, tp.tv_sec, tp.tv_nsec);

  float stim_size = 24;	// Degrees
  int twidth = (int) ceilf (setup.deg2pix (stim_size));

  // Initialise the graphic system
  xp.egl_init (width, height, fullscreen, argv[0],
	       nframes, twidth, twidth);


  cout << endl;
  // Run the session
  if (! xp.run_session ())
    return 1;


  // Cleanup
  xp.egl_cleanup ();

  return 0;
}

// vim:sw=2:
