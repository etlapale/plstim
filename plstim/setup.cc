#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

#include <yaml-cpp/yaml.h>


#include "setup.h"
using namespace plstim;


const std::string Setup::default_source ("setup.yaml");


Setup::Setup ()
{
}

bool
Setup::load (const std::string& filename)
{
  // Open the setup file
  ifstream fin (filename.c_str ());
  if (! fin.good ()) {
    cerr << "error: setup file " << filename
	 << " not found" << endl;
    return false;
  }

  // Load the YAML content
  YAML::Parser parser (fin);
  YAML::Node doc;
  if (! parser.GetNextDocument (doc)) {
    cerr << "error: empty setup file " << filename << endl;
    return false;
  }

  // Load setup parameters
  string tmp;
  doc["resolution"] >> tmp;
  stringstream ss(tmp);
  ss >> resolution[0] >> resolution[1];
  doc["size"] >> tmp;
  ss.clear ();
  ss.str (tmp);
  ss >> size[0] >> size[1];
  auto node_lum = doc.FindValue ("luminance");
  doc["luminance"] >> tmp;
  ss.clear ();
  ss.str (tmp);
  ss >> luminance[0] >> luminance[1];
  doc["distance"] >> distance;

  auto node_refresh = doc.FindValue ("refresh");
  if (node_refresh)
    refresh = node_refresh->to<float> ();
  else
    refresh = 0;
  cout << "Refresh rate: " << refresh << endl;

  // Compute vertical and horizontal resolutions
  float hres = resolution[0] / size[0];
  float vres = resolution[1] / size[1];
  float err = fabsf((hres - vres) / (hres + vres));
  if (err > 0.01) {
    cerr << "error: too much difference between horizontal ("
         << hres << ") and vertical (" << vres
	 << ") resolutions" << endl;
    return false;
  }
  px_mm = (hres + vres) / 2.0;

  return true;
}
