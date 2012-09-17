lorenceau-lines: lorenceau-lines.cc plstim/setup.cc plstim/setup.h
	$(CXX) -o $@ $(CXXFLAGS) -std=c++11 -I. $< plstim/setup.cc `pkg-config --cflags --libs tclap` `pkg-config --cflags --libs yaml-cpp` `pkg-config --cflags --libs cairomm-png-1.0`
