// plstim/qexperiment.h – Manage a real experiment

#ifndef __PLSTIM_QEXPERIMENT_H
#define __PLSTIM_QEXPERIMENT_H

/*#include <cmath>
#include <map>
#include <string>
#include <vector>*/

#include <QApplication>
#include <QMainWindow>


#include "glwidget.h"
#include "utils.h"


namespace plstim
{
  class Message {
  public:
    enum Type {
      WARNING,
      ERROR
    };
  public:
    Message (Type t, const QString& msg);
  public:
    Type type;
    QString msg;
    QList<QWidget*> widgets;
  };

  class QExperiment : public QObject
  {
  Q_OBJECT
  protected:
    /// Associated Qt application
    QApplication app;
    QMainWindow win;
    MyGLWidget* glwidget;

    /// Application settings
    QSettings* settings;

    QDesktopWidget dsk;

    /// List of current messages
    QList<Message*> messages;

    Message* res_msg;
    Message* match_res_msg;

    /// Output display to be used
    std::string routput;

    /// Desired monitor refresh rate in Hertz
    float wanted_frequency;

    /// Trial duration in ms
    float dur_ms;

    /**
     * Minimal number of buffer swaps between frames.
     *
     * Set to 0 to disable synchronisation.
     */
    int swap_interval;

    /// Location of the current texture
    int texloc;

    /// Texture width (2ⁿ)
    int tex_width;

    /// Texture height (2ⁿ)
    int tex_height;

    /// Current trial number
    int current_trial;

  public:
    /// Number of trials in a session
    int ntrials;

    /// Number of frames per trial
    int nframes;

    /// Trial frames as OpenGL textures
    //GLuint* tframes;

    /// Special frames required by the protocol
    //std::map<std::string,GLuint> special_frames;

  protected:

    void error (const std::string& msg);

    /**
     * Copy a pixel surface into an OpenGL texture.
     * The destination texture must have been already created
     * with gen_frame(), and its size should match the other
     * textures (tex_width×tex_height).  The source surface can be
     * destroyed after calling this function.
     */
    //bool copy_to_texture (const GLvoid* src, GLuint dest);
    
    void add_message (Message* msg);

    void remove_message (Message* msg);

  public:

    QExperiment (int& argc, char** argv);
    virtual ~QExperiment ();

    bool exec ();

    /// Run a single trial
    virtual bool run_trial () = 0;

    /// Generate frames for a single trial
    virtual bool make_frames () = 0;

    /// Show the frames loaded in ‘tframes’
    bool show_frames ();

    /// Show a special frame
    bool show_frame (const std::string& frame_name);

    /// Define a special frame
    bool gen_frame (const std::string& frame_name);

    /// Clear the screen
    bool clear_screen ();

    /// Wait for any key to be pressed
    bool wait_any_key ();

    /// Wait for a key press in a given set
    /*bool wait_for_key (const std::vector<KeySym>& accepted_keys,
		       KeySym* pressed_key);*/

    /// Convert a degrees distance to pixels
    float deg2pix (float dst) const {
      return 2*distance*tan(radians(dst)/2)*px_mm;
    }

  public slots:

    /**
     * Run a session of multiple trials.
     * Sessions may run asynchronously, so this method may return
     * before the session being finished.
     */
    void run_session ();

  protected:
    bool add_frame (const std::string& name, const QImage& img);

  protected slots:
    //void screen_param_changed ();
    void setup_param_changed ();
    void update_setup ();
    void update_converters ();
    void about_to_quit ();
    void quit ();
    void glwidget_gl_initialised ();
    void gl_resized (int w, int h);
    void normal_screen_restored ();

  protected:
    bool save_setup;
    QToolBar* toolbar;
    QSplitter* splitter;
    QComboBox* setup_cbox;
    QSpinBox* screen_sbox;
    QLineEdit* res_x_edit;
    QLineEdit* res_y_edit;
    QLineEdit* phy_width_edit;
    QLineEdit* phy_height_edit;
    QLineEdit* dst_edit;
    QLineEdit* lum_min_edit;
    QLineEdit* lum_max_edit;
    QLineEdit* refresh_edit;

    float distance;
    float px_mm;

    bool glwidget_initialised;

    QByteArray splitter_state;

    /**
     * Waiting for a fullscreen event before starting the session.
     */
    bool waiting_fullscreen;

  signals:
    void setup_updated ();
  };
}

#endif

// vim:filetype=cpp:
