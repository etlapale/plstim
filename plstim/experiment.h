// plstim/experiment.h – Manage a real experiment

#ifndef __PLSTIM_EXPERIMENT_H
#define __PLSTIM_EXPERIMENT_H

#include <cmath>
#include <string>
#include <vector>

// OpenGL ES 2.0 through EGL 2.0
#include <EGL/egl.h>
#include <GLES2/gl2.h>



#include "utils.h"


namespace plstim
{
  class Experiment
  {
  protected:
    EGLNativeDisplayType nat_dpy;
    EGLNativeWindowType nat_win;

    EGLDisplay egl_dpy;
    EGLConfig config;
    EGLContext ctx;
    EGLSurface sur;

    /**
     * Minimal number of buffer swaps between frames.
     *
     * Set to 0 to disable synchronisation.
     */
    int swap_interval;

    /// Location of the current texture
    int texloc;

    int twidth;

    int theight;

  public:
    /// Number of trials in a session
    int ntrials;

    /// Number of frames per trial
    int nframes;

    /// Frames as OpenGL textures
    GLuint* tframes;

  protected:
    void error (const std::string& msg);

  public:

    Experiment ();

    bool egl_init (int width, int height, bool fullscreen,
		   const std::string& title,
		   int nframes,
		   int texture_width, int texture_height);
 

    void egl_cleanup ();

    bool run_session ();

    /// Run a single trial
    virtual bool run_trial () = 0;

    /// Generate frames for a single trial
    virtual bool make_frames () = 0;

    /// Show the frames loaded in ‘tframes’
    bool show_frames ();

    /// Clear the screen
    bool clear_screen ();

    /// Wait for any key to be pressed
    bool wait_any_key ();

    /// Wait for a key press in a given set
    bool wait_for_key (const std::vector<int>& accepted_keys,
		       int* pressed_key);
  };
}

#endif

// vim:filetype=cpp:
