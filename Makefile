PLSTIM_HDR=$(wildcard plstim/*.h)
PLSTIM_SRC=$(wildcard plstim/*.cc)

lorenceau-experiment: lorenceau-experiment.cc $(PLSTIM_HDR) $(PLSTIM_SRC)
	$(CXX) -o $@ $(CXXFLAGS) -I. -std=c++11 lorenceau-experiment.cc $(PLSTIM_SRC) `pkg-config --cflags --libs tclap` `pkg-config --cflags --libs glesv2` `pkg-config --cflags --libs egl` `pkg-config --cflags --libs libpng` `pkg-config --cflags --libs yaml-cpp` `pkg-config --cflags --libs cairomm-png-1.0` -lX11 -lrt -lhdf5_cpp -lhdf5

egl-anim: egl-anim.cc
	$(CXX) -o $@ $(CXXFLAGS) -std=c++11 $< `pkg-config --cflags --libs tclap` `pkg-config --cflags --libs glesv2` `pkg-config --cflags --libs egl` `pkg-config --cflags --libs libpng` -lX11 -lrt

lorenceau-lines: lorenceau-lines.cc plstim/setup.cc plstim/setup.h
	$(CXX) -o $@ $(CXXFLAGS) -std=c++11 -I. $< plstim/setup.cc `pkg-config --cflags --libs tclap` `pkg-config --cflags --libs yaml-cpp` `pkg-config --cflags --libs cairomm-png-1.0`
