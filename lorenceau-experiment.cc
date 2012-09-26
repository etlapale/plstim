// lorenceau-experiment.cc – Experiment of Lorenceau et al (1993)

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include <time.h>
#include <unistd.h>
#include <sys/time.h>


#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

// Command line parsing
#include <tclap/CmdLine.h>

// Drawing library
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
using namespace Cairo;

// Storage library
#include <H5Cpp.h>
using namespace H5;


// Stimulus library
#include <plstim/plstim.h>
using namespace plstim;


// If possible, use a clock not sensible to adjtime or ntp
// (Linux only)
#define CLOCK CLOCK_MONOTONIC_RAW

// Otherwise switch back to a clock at least not sensible to
// administrator clock settings
//#define CLOCK CLOCK_MONOTONIC


#define LORENCEAU_CONFIG_CW		(1<<0)
#define LORENCEAU_CONFIG_CONTROL	(1<<1)
#define LORENCEAU_CONFIG_UP		(1<<2)

struct SessionResult
{
  time_t begin_sec;
  long begin_nsec;
  /// Bitfield of the configuration
  uint8_t config;
  /// Subject response (up or down)
  uint8_t response;
  time_t answer_sec;
  long answer_nsec;
};

class LorenceauExperiment : public Experiment
{
protected:
  /// Associated hardware setup
  Setup* setup;

  /// Result file
  H5::H5File* hf;

  /// Result dataset
  H5::DataSet dset;

  std::vector<KeySym> answer_keys;

  /// Desired monitor refresh rate in Hertz
  float wanted_frequency;

  /// Trial duration in milliseconds
  float dur_ms;

  /// Foreground colour
  float fg;
  /// Background colour
  float bg;
  /// Line length
  float ll;
  /// Line width
  float lw;

  /// Diameter of the aperture through which lines are moving
  float aperture_diam;

  float spacing;

  /// Aperture radius
  float radius;

  /// Fixation dot radius
  float fix_radius;

  /// Orientation of the lines
  bool cw;

  /// Real motion direction
  bool up;

  /// Control or test experiment
  bool control;

public:
  LorenceauExperiment (Setup* s, const std::string& output,
		       int win_width, int win_height, bool fullscreen,
		       const std::string& win_title,
		       int aperture_diameter,
		       
		       const std::string& subject);
  virtual ~LorenceauExperiment ();
  virtual bool run_trial ();
  virtual bool make_frames ();
};


LorenceauExperiment::LorenceauExperiment (Setup* s,
    const std::string& result_file,
    int win_width, int win_height, bool fullscreen,
    const std::string& win_title, int aperture_diameter,

    const std::string& subject)
  : setup (s), 
    wanted_frequency (30), dur_ms (332),
    aperture_diam (aperture_diameter)
{
  ntrials = 400;

  // Compute the required number of frames in a trial
  int coef = (int) nearbyintf (setup->refresh / wanted_frequency);
  if ((setup->refresh/coef - wanted_frequency)/wanted_frequency > 0.01) {
    cerr << "error: cannot set monitor frequency at 1% of the desired frequency" << endl;
    exit (1);
  }

  fix_radius = setup->deg2pix (3./60);
  cout << "Fixation point radius: " << fix_radius << endl;

  nframes = (int) nearbyintf ((setup->refresh/coef)*(dur_ms/1000.));
  cout << "Setting the number of frames to" << nframes << endl;

  // Possible answers in a trial
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

  // Set the bar positions
  spacing = setup->deg2pix (1);

  // Initialise the graphic system
  egl_init (win_width, win_height, fullscreen, win_title,
	    aperture_diameter, aperture_diameter);


  radius = aperture_diam/2;

  // Initialise the special frames
  gen_frame ("fixation");
  gen_frame ("question");

  // Create the special frames
  auto sur = ImageSurface::create (Cairo::FORMAT_RGB24,
				   tex_width, tex_height);
  auto cr = Cairo::Context::create (sur);
  cr->set_fill_rule (Cairo::FILL_RULE_EVEN_ODD);
  
  // Pre-trial fixation frame
  cr->set_source_rgb (bg, bg, bg);
  cr->paint ();
  // Fixation point
  cr->set_source_rgb (0, 0, 1);
  cr->arc (tex_width/2, tex_height/2, fix_radius, 0, 2*M_PI);
  cr->fill ();
  // Aperture
  cr->rectangle (0, 0, tex_width, tex_height);
  cr->arc (tex_width/2, tex_height/2, radius, 0, 2*M_PI);
  cr->set_source_rgb (0, 0, 0);
  cr->fill ();
  // Copy into OpenGL texture
  if (! copy_to_texture (sur->get_data (),
			 special_frames["fixation"]))
    exit (1);
  
  // Question frame
  float key_size = setup->deg2pix (2);
  float cx = tex_width/2;
  float cy = tex_height/2;
  cr->set_source_rgb (bg, bg, bg);
  cr->paint ();
  // Key
  cr->set_line_width (1.0);
  cr->set_source_rgb (fg, fg, fg);
  cr->rectangle (cx-2*key_size, cy-key_size/2, key_size, key_size);
  cr->rectangle (cx+key_size, cy-key_size/2, key_size, key_size);
  cr->stroke ();
  // Arrows
  cr->set_line_width (4.0);
  cr->move_to (cx-1.5*key_size, cy);
  cr->rel_line_to (0, 0.25*key_size);
  cr->move_to (cx+1.5*key_size, cy);
  cr->rel_line_to (0, -0.25*key_size);
  cr->stroke ();
  cr->move_to (cx-1.5*key_size, cy-0.25*key_size);
  cr->rel_line_to (-key_size/8, 0.25*key_size);
  cr->rel_line_to (key_size/4, 0);
  cr->move_to (cx+1.5*key_size, cy+0.25*key_size);
  cr->rel_line_to (-key_size/8, -0.25*key_size);
  cr->rel_line_to (key_size/4, 0);
  cr->fill ();
  // Aperture
  cr->rectangle (0, 0, tex_width, tex_height);
  cr->arc (cx, cy, radius, 0, 2*M_PI);
  cr->set_source_rgb (0, 0, 0);
  cr->fill ();
  // Copy into OpenGL texture
  if (! copy_to_texture (sur->get_data (),
			 special_frames["question"]))
    exit (1);

  // Create an output result file
  if (access (result_file.c_str (), F_OK) == 0) {
    error ("result file already existing");
    exit (1);
  }
  hf = new H5File (result_file.c_str (), H5F_ACC_TRUNC);
  CompType session_type (sizeof (SessionResult));
  session_type.insertMember ("begin_sec",
			     HOFFSET (SessionResult, begin_sec),
			     PredType::NATIVE_LONG);
  session_type.insertMember ("begin_nsec",
			     HOFFSET (SessionResult, begin_nsec),
			     PredType::NATIVE_LONG);
  session_type.insertMember ("config",
			     HOFFSET (SessionResult, config),
			     PredType::NATIVE_UINT8);
  session_type.insertMember ("response",
			     HOFFSET (SessionResult, response),
			     PredType::NATIVE_UINT8);
  session_type.insertMember ("answer_sec",
			     HOFFSET (SessionResult, answer_sec),
			     PredType::NATIVE_LONG);
  session_type.insertMember ("answer_nsec",
			     HOFFSET (SessionResult, answer_nsec),
			     PredType::NATIVE_LONG);
  hsize_t htrials = ntrials;
  DataSpace dspace (1, &htrials);
  dset = hf->createDataSet ("session-1", session_type, dspace);

  // Save session metadata
  StrType str_type (PredType::C_S1, subject.size ());
  DataSpace scalar_space (H5S_SCALAR);
  dset.createAttribute ("subject", str_type, scalar_space)
    .write (str_type, subject.c_str ());
  time_t now = time (NULL);
  dset.createAttribute ("date", PredType::NATIVE_LONG, scalar_space)
    .write (PredType::NATIVE_LONG, &now);
  struct utsname ubuf;
  if (uname (&ubuf) != 0) {
    error ("cannot use uname()");
    exit (1);
  }
  StrType sysname_type (PredType::C_S1, strlen (ubuf.sysname));
  dset.createAttribute ("sysname", sysname_type, scalar_space)
    .write (sysname_type, ubuf.sysname);
  StrType nodename_type (PredType::C_S1, strlen (ubuf.nodename));
  dset.createAttribute ("nodename", nodename_type, scalar_space)
    .write (nodename_type, ubuf.nodename);
  // Code commit
  StrType commit_type (PredType::C_S1, 40);
  dset.createAttribute ("commit", commit_type, scalar_space)
    .write (commit_type, PLSTIM_COMMIT);
  hf->flush (H5F_SCOPE_GLOBAL);
}

LorenceauExperiment::~LorenceauExperiment ()
{
  delete hf;
}


bool
LorenceauExperiment::run_trial ()
{
  struct timespec tp_start, tp_stop;

  make_frames ();

  // Wait for a key press before running the trial
  show_frame ("fixation");
  if (! wait_any_key ())
    return false;

  //cout << "Starting" << endl;
  // Mark the beginning time
  clock_gettime (CLOCK, &tp_start);

  // Display each frame of the trial
  if (! show_frames ())
    return false;

  // Mark the end time
  clock_gettime (CLOCK, &tp_stop);

  // Display the elapsed time
  printf ("start: %lds %ldns\n",
	  tp_start.tv_sec, tp_start.tv_nsec);
  printf ("stop:  %lds %ldns\n",
	  tp_stop.tv_sec, tp_stop.tv_nsec);

  // Wait for a key press at the end of the trial
  show_frame ("question");
  KeySym pressed_key;
  if (! wait_for_key (answer_keys, &pressed_key))
    return false;

  cout << "Answer: " << (pressed_key == XK_Up ? "up" : "down") << endl;

  // Write the results
  hf->flush (H5F_SCOPE_GLOBAL);
  
  return true;
}

bool
LorenceauExperiment::make_frames ()
{
  int offx = (tex_width-aperture_diam)/2;
  int offy = (tex_height-aperture_diam)/2;
  // Create a Cairo context to draw the frames
  auto sur = ImageSurface::create (Cairo::FORMAT_RGB24,
				   tex_width, tex_height);
  auto cr = Cairo::Context::create (sur);

  // Line speed and direction
  cout << "Clockwise: " << cw << endl
       << "Control: " << control << endl
       << "Up: " << up << endl;
  float orient = radians (cw ? 110 : 70);
  int bx = (int) (ll*cos(orient));
  int by = (int) (ll*sin(orient));
  int sx = abs (bx);
  int sy = abs (by);
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
  cout << "sx+spacing: " << sx << "+" << spacing << endl;

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
    for (int x = offx+i*dx - (sx+spacing); x < tex_width+sx+spacing; x+=sx+spacing) {
      for (int y = offy+i*dy; y < tex_height+sy+spacing; y+=sy+spacing) {
	cr->move_to (x, y);
	cr->rel_line_to (bx, by);
      }
    }
    cr->stroke ();
    
    // Fixation point
    cr->set_source_rgb (0, 0, 1);
    cr->arc (tex_width/2, tex_height/2, fix_radius, 0, 2*M_PI);
    cr->fill ();

    // Aperture
    cr->rectangle (0, 0, tex_width, tex_height);
    cr->arc (tex_width/2, tex_height/2, radius, 0, 2*M_PI);
    cr->set_source_rgb (0, 0, 0);
    cr->fill ();

    // Save the frame as PNG
#if 0
    char buf[20];
    const char* pat = "output-%04d.png";
    snprintf (buf, 20, pat, i);
    sur->write_to_png (buf);
#endif

    // Copy into OpenGL texture
    if (! copy_to_texture (sur->get_data (), tframes[i]))
      return false;
  }

  cout << nframes << " frames loaded" << endl;

  return true;
}


int
main (int argc, char* argv[])
{
  int res;

  unsigned int win_width, win_height;

  std::string output;
  std::string subject;
  
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
    TCLAP::ValueArg<string> arg_output ("o", "output",
	"Output file to write results", false, "",
	"PATH");
    cmd.add (arg_output);
    TCLAP::ValueArg<string> arg_subject ("s", "subject",
	"Subject initials", true, "", "SUBJECT");
    cmd.add (arg_subject);
    TCLAP::ValueArg<std::string> setup_arg ("S", "setup",
	"Hardware setup configuration", false, Setup::default_source, "SETUP");
    cmd.add (setup_arg);
    // Parser the command line arguments
    cmd.parse (argc, argv);

    // Store the setup configuration
    if (! setup.load (setup_arg.getValue ()))
      return 1;

    // Generate a new HDF5 filename if none exists
    output = arg_output.getValue ();

    subject = arg_subject.getValue ();

    // Parse the geometry
    auto geom = arg_geom.getValue ();
    if (geom.empty ()) {
      win_width = setup.resolution[0];
      win_height = setup.resolution[1];
      fullscreen = true;
    }
    else {
      size_t sep = geom.find ('x');
      if (sep == geom.npos) {
	cerr << "error: invalid stimulus dimension: " << geom << endl;
	return 1;
      }
      win_width = atof (geom.c_str ());
      win_height = atof (geom.c_str () + sep + 1);
      if (win_width == 0 || win_height == 0) {
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
  // Get the clock resolution
  struct timespec tp;
  res = clock_getres (CLOCK, &tp);
  if (res < 0) {
    perror ("clock_getres");
    return 1;
  }
  printf ("System clock %d resolution: %lds %ldns\n",
	  CLOCK, tp.tv_sec, tp.tv_nsec);
  cout << endl;


  // Experiment
  float diameter = 24;	// Degrees
  int aperture_diameter = (int) ceilf (setup.deg2pix (diameter));
  LorenceauExperiment xp (&setup, output,
			  win_width, win_height, fullscreen,
			  argv[0], aperture_diameter,
			  
			  subject);
  // Run the session
  if (! xp.run_session ())
    return 1;

  return 0;
}

// vim:sw=2:
