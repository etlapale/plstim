// simple-experiment.cc – Experiment of Lorenceau et al (1993) 

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

// Stimulus library
#include <plstim/plstim.h>
using namespace plstim;


class LorenceauExperiment : public QExperiment
{
public:
  LorenceauExperiment (int & argc, char** argv,
		       Setup* s, const std::string& output,
		       int win_width, int win_height, bool fullscreen,
		       const std::string& routput,
		       const std::string& win_title,
		       int aperture_diameter,
		       const std::string& subject);
  virtual ~LorenceauExperiment ();
  virtual bool run_trial ();
  virtual bool make_frames ();
};


LorenceauExperiment::LorenceauExperiment (int & argc, char** argv,
					  Setup* s,
    const std::string& result_file,
    int win_width, int win_height, bool fullscreen,
    const std::string& _routput,
    const std::string& win_title, int aperture_diameter,

    const std::string& subject)
  : QExperiment (argc, argv)
{
  ntrials = 10;
#if 0
  // Initialise the random number generator
  twister.seed (time (NULL));
  for (int i = 0; i < 10000; i++)
    (void) bin_dist (twister);

  fix_radius = setup->deg2pix (3./60);
  cout << "Fixation point radius: " << fix_radius << endl;

  // Possible answers in a trial
  answer_keys.push_back (XK_Up);
  answer_keys.push_back (XK_Down);

  // Stimulus configuration
  float lw_deg = sec2deg (1.02);
  lw = setup->deg2pix (lw_deg);
  cout << "Line width is " << lw_deg << " degrees, or "
       << lw << " pixels" << endl;
  float bg_cd = 20;
  bg = setup->lum2px (bg_cd);
  cout << "Background luminance is " << bg_cd << " cd/m², or "
       << bg << " in [0,1]" << endl;

  // Set the bar positions
  spacing = setup->deg2pix (1);

  radius = aperture_diam/2;
#endif

#if 0
  // Initialise the graphic system
  bool ok = egl_init (win_width, win_height, fullscreen, win_title,
		      aperture_diameter, aperture_diameter);
  if (! ok)
    exit (1);
#endif

#if 0
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
  float cx = tex_width/2;
  float cy = tex_height/2;
  cr->set_source_rgb (bg, bg, bg);
  cr->paint ();
  // Aperture
  cr->rectangle (0, 0, tex_width, tex_height);
  cr->arc (cx, cy, radius, 0, 2*M_PI);
  cr->set_source_rgb (0, 0, 0);
  cr->fill ();
  // Copy into OpenGL texture
  if (! copy_to_texture (sur->get_data (),
			 special_frames["question"]))
    exit (1);
#endif
}

LorenceauExperiment::~LorenceauExperiment ()
{
}

bool
LorenceauExperiment::run_trial ()
{
  cout << "RUN TRIAL (NYI)" << endl;
#if 0
  // Set a random trial condition
  up = bin_dist (twister);
  cw = bin_dist (twister);
  control = bin_dist (twister);
  // Contrast
  // C%→L: C₁₂→13.44, C₂₅→15, C₃₉→16.68, C₅₂→18.24, C₇₀→20.4
  float fg_cd = luminances [lum_dist (twister)];
  fg = setup->lum2px (fg_cd);
  cout << "Foreground luminance is " << fg_cd << " cd/m², or "
       << fg << " in [0,1], or "
       << (int) (fg*255) << " in [0,255]" << endl;
  // Line length
  float ll_deg = 2.7;
  ll = setup->deg2pix (ll_deg);
  cout << "Line length is " << ll_deg << " degrees, or "
       << ll << " pixels" << endl;

  make_frames ();

  // Wait for a key press before running the trial
  struct timespec tp_fixation;
  clock_gettime (CLOCK, &tp_fixation);
  show_frame ("fixation");
#if !NO_INTERACTIVE
  if (! wait_any_key ())
    return false;
#endif

  // Display each frame of the trial
  clear_screen ();
  struct timespec tp_frames;
  clock_gettime (CLOCK, &tp_frames);
  if (! show_frames ())
    return false;

  // Wait for a key press at the end of the trial
  struct timespec tp_question;
  clock_gettime (CLOCK, &tp_question);
  printf ("start: %lds %ldns\n",
	  tp_frames.tv_sec, tp_frames.tv_nsec);
  printf ("stop:  %lds %ldns\n",
	  tp_question.tv_sec, tp_question.tv_nsec);
  show_frame ("question");
  KeySym pressed_key;
#if !NO_INTERACTIVE
  if (! wait_for_key (answer_keys, &pressed_key))
    return false;
#endif

  struct timespec tp_answer;
  clock_gettime (CLOCK, &tp_answer);

  // Prepare results
  uint8_t config = 0;
  if (cw) config |= LORENCEAU_CONFIG_CW;
  if (control) config |= LORENCEAU_CONFIG_CONTROL;
  if (up) config |= LORENCEAU_CONFIG_UP;
  SessionResult result = {
    tp_fixation.tv_sec, tp_fixation.tv_nsec,
    tp_frames.tv_sec, tp_frames.tv_nsec,
    tp_question.tv_sec, tp_question.tv_nsec,
    tp_answer.tv_sec, tp_answer.tv_nsec,
    config,
    fg_cd,
    ll_deg,
    (int) dur_ms ,
    pressed_key == XK_Up,
  };

  // Write result slab
  hsize_t one = 1;
  DataSpace fspace = dset.getSpace();
  hsize_t hframe = current_trial;
  fspace.selectHyperslab (H5S_SELECT_SET, &one, &hframe);
  DataSpace mspace (1, &one);
  dset.write (&result, session_type, mspace, fspace);

  // Flush to commit trial
  hf->flush (H5F_SCOPE_GLOBAL);
#endif
  
  return true;
}

bool
LorenceauExperiment::make_frames ()
{
  cout << "MAKE FRAMES (NYI)" << endl;
#if 0
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
#endif

  return true;
}


int
main (int argc, char* argv[])
{
  int res;

  unsigned int win_width, win_height;

  std::string output;
  std::string subject;
  std::string rout;
  
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
#ifdef HAVE_XRANDR
    TCLAP::ValueArg<string> arg_rout ("o", "output",
	"RandR output", false, "", "OUTPUT");
    cmd.add (arg_rout);
#endif // HAVE_XRANDR
    TCLAP::ValueArg<string> arg_subject ("s", "subject",
	"Subject initials", true, "", "SUBJECT");
    cmd.add (arg_subject);
    TCLAP::ValueArg<std::string> setup_arg ("S", "setup",
	"Hardware setup configuration", false, Setup::default_source, "SETUP");
    cmd.add (setup_arg);
    TCLAP::UnlabeledValueArg<string> arg_output ("results",
	"File to write results", false, "", "PATH");
    cmd.add (arg_output);
    // Parser the command line arguments
    cmd.parse (argc, argv);

    // TODO: generate a new HDF5 filename if none exists
    output = arg_output.getValue ();

    subject = arg_subject.getValue ();

#ifdef HAVE_XRANDR
    // Get the RandR output to use
    rout = arg_rout.getValue ();
#endif // HAVE_XRANDR

    // Store the setup configuration
    if (! setup.load (setup_arg.getValue ()))
      return 1;

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

  // Experiment
  float diameter = 24;	// Degrees
  int aperture_diameter = (int) ceilf (setup.deg2pix (diameter));
  LorenceauExperiment xp (argc, argv,
			  &setup, output,
			  win_width, win_height, fullscreen,
			  rout,
			  argv[0], aperture_diameter,
			  subject);
  // Run the session
  xp.exec ();
  //if (! xp.run_session ())
    //return 1;

  return 0;
}

// vim:sw=2:
