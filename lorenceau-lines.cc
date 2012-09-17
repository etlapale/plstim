// lorenceau-lines.cc – Generate moving line stimuli.

#include <stdio.h>

#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

// Command line parsing
#include <tclap/CmdLine.h>

// Stimuli library
#include <plstim/setup.h>
using namespace plstim;

// Graphics library
#include <cairommconfig.h>
#include <cairomm/context.h>
#include <cairomm/surface.h>
using namespace Cairo;


/**
 * Convert a distance in seconds to degrees of visual field.
 */
static inline float
sec2deg (float dst)
{
  return dst / 60;
}

/**
 * Return the object luminance to the specified contrast.
 */
static inline float
luminance_from_weber_contrast (float contrast, float background)
{
  return contrast * background + background;
}


int
main (int argc, char* argv[])
{
  // Physical setup configuration
  Setup setup;

  try {
    // Setup the command line parser
    TCLAP::CmdLine cmd ("Generate moving line stimuli", ' ', "0.1");
    TCLAP::ValueArg<float> dir_arg ("d", "direction",
	"Motion direction", false, 0.0f, "DIR");
    cmd.add (dir_arg);
    TCLAP::ValueArg<float> freq_arg ("f", "frequency",
	"Stimulus refresh rate", false, 0.0f, "RATE");
    cmd.add (freq_arg);
    TCLAP::ValueArg<string> dim_arg ("g", "geometry",
	"Stimulus dimension in degrees", false, "", "DIM");
    cmd.add (dim_arg);
    TCLAP::ValueArg<string> out_arg ("o", "output",
	"Output frames pattern", false, "output-%04d.png", "WIDTHxHEIGHT");
    cmd.add (out_arg);
    TCLAP::ValueArg<std::string> setup_arg ("s", "setup",
	"Hardware setup configuration", false, Setup::default_source, "SETUP");
    cmd.add (setup_arg);
    TCLAP::ValueArg<int> dur_arg ("t", "duration",
	"Stimulus duration in frames", false, 1, "DIR");
    cmd.add (dur_arg);

    // Parser the command line arguments
    cmd.parse (argc, argv);

    // Store the setup configuration
    if (! setup.load (setup_arg.getValue ()))
      return 1;
    
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
    string output_pattern = out_arg.getValue ();


    // Compute stimulus geometry
    float ll_deg = 2.7;
    float ll = setup.deg2pix (ll_deg);
    cout << "Line length is " << ll_deg << " degrees, or "
         << ll << " pixels" << endl;
    float lw_deg = sec2deg (1.02);
    float lw = setup.deg2pix (lw_deg);
    cout << "Line width is " << lw_deg << " degrees, or "
         << lw << " pixels" << endl;
    float bg_cd = 12;
    float bg = setup.lum2px (bg_cd);
    cout << "Background luminance is " << bg_cd << " cd/m², or "
         << bg << " in [0,1]" << endl;
    // C%→L: C₁₂→13.44, C₂₅→15, C₃₉→16.68, C₅₂→18.24, C₇₀→20.4
    float fg_cd = 100;//16.68;
    float fg = setup.lum2px (fg_cd);
    cout << "Foreground luminance is " << fg_cd << " cd/m², or "
         << fg << " in [0,1]" << endl;
    int nframes = dur_arg.getValue ();
    cout << "Duration is " << (setup.frm2sec (nframes) * 1000)
         << " ms, or " << nframes << " frames" << endl;
    auto geom = dim_arg.getValue ();
    double stim_width, stim_height;
    if (geom.empty ()) {
      stim_width = setup.pix2deg (setup.resolution[0]);
      stim_height = setup.pix2deg (setup.resolution[1]);
    }
    else {
      size_t sep = geom.find ('x');
      if (sep == geom.npos) {
	cerr << "error: invalid stimulus dimension: " << geom << endl;
	return 1;
      }
      stim_width = atof (geom.c_str ());
      stim_height = atof (geom.c_str () + sep + 1);
      if (stim_width == 0 || stim_height == 0) {
	cerr << "error: invalid stimulus dimension: " << geom << endl;
	return 1;
      }
    }
    int width = (int) setup.deg2pix (stim_width);
    int height = (int) setup.deg2pix (stim_height);
    cout << "Dimension is " << stim_width << "×" << stim_height
         << " degrees, or " << width << "×" << height << " pixels"
	 << endl;

    // Create a Cairo image
    auto sur = ImageSurface::create (Cairo::FORMAT_RGB24,
				     width, height);

    auto buflen = output_pattern.size () + 11;
    auto filename = new char[buflen];

    // Bar geometry
    float orient = radians (70);
    int bx = (int) (ll*cos(orient));
    int by = (int) (ll*sin(orient));
    cout << bx << " " << by << endl;
    int sx = abs (bx);
    int sy = abs (by);

    // Set the bar positions
    int spacing = setup.deg2pix (1);

    // Motion
    bool control = true;
    bool up = false;

    float direction = fmod (orient
      + (orient < M_PI/2 ? -1 : +1) * (M_PI/2)	// Orientation
      + (up ? 0 : 1) * M_PI			// Up/down
      + (orient < M_PI/2 ? -1 : 1) * (control ? 0 : 1) * radians (140)
      , 2*M_PI);
    cout << "Line direction: " << degrees (direction) << "°" << endl;

    float speed = setup.ds2pf (6.5);

    int dx = (int) (speed*cos (direction));
    int dy = (int) (speed*sin (direction));
    cout << "Line speed is 6.5 deg/s, or "
	 << speed << " px/frame ("
	 << (dx >= 0 ? "+" : "") << dx
	 << (dy >= 0 ? "-" : "") << dy
	 << ")" << endl;
    exit (42);

    // Render each frame
    auto cr = Cairo::Context::create (sur);
    for (int t = 0; t < nframes; t++) {
      // Background
      cr->set_source_rgb (bg, bg, bg);
      cr->paint ();

      // Lines
      cr->set_line_width (lw);
      cr->set_source_rgb (fg, fg, fg);
      for (int x = t*dx; x < width; x+=sx+spacing) {
	for (int y = t*dy; y < height; y+=sy+spacing) {
	  cr->move_to (x, y);
	  cr->rel_line_to (bx, by);
	}
      }
      cr->stroke ();

      // Write the frame on disk
      snprintf (filename, buflen, output_pattern.c_str (), t);
      sur->write_to_png (filename);
      /*
      FILE* fp = fopen (filename, "wb");
      fwrite (sur->get_data (), sur->get_height () * sur->get_stride (), 1, fp);
      fclose (fp);*/
    }

    // Cleanup
    delete [] filename;

    // Catch command line arguments errors
  } catch (TCLAP::ArgException& e) {
    cerr << "error: could not parse argument "
         << e.argId () << ": " << e.error () << endl;
  }

  return 0;
}
