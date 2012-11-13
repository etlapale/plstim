// plstim/qexperiment.h – Manage a real experiment

#ifndef __PLSTIM_QEXPERIMENT_H
#define __PLSTIM_QEXPERIMENT_H

#include <set>

#include <QApplication>
#include <QMainWindow>
#include <QtScript>


#include "glwidget.h"
#include "qswrappers.h"
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
    Message (Message::Type t, const QString& msg);
  public:
    Message::Type type;
    QString msg;
    QList<QWidget*> widgets;
  };

  class Page : public QObject
  {
  Q_OBJECT
  public:
    enum Type {
      SINGLE,
      FRAMES
    };
    Q_ENUMS (Type)
  public:
    Page::Type type;
    QString title;
    bool wait_for_key;
    std::set<int> accepted_keys;
  public:
    Page (Page::Type t, const QString& title);
    virtual ~Page ();
    void accept_key (int key);
  public:
    void make_active ();
    void emit_key_pressed (QKeyEvent* evt);
  signals:
    /**
     * Called when the page is the one currently displayed.
     */
    void page_active ();

    void key_pressed (QKeyEvent* evt);
  };

  class QExperiment : public QObject
  {
    Q_OBJECT
    Q_PROPERTY (float monitor_rate READ monitor_rate)
    Q_PROPERTY (int textureSize READ textureSize WRITE setTextureSize)
  protected:
    /// Associated Qt application
    QApplication app;
    QMainWindow win;
    MyGLWidget* glwidget;

    /// Application settings
    QSettings* settings;

    QDesktopWidget dsk;

    ColorPrototype color_proto;
    PainterPrototype painter_proto;
    PainterPathPrototype painterpath_proto;

    /// List of current messages
    QList<Message*> messages;

    Message* res_msg;
    Message* match_res_msg;

    /// Pages composing a trial
    std::vector<Page*> pages;

    /// Current page in a trial
    int current_page;

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
    int tex_size;

    /// Current trial number
    int current_trial;

  public:
    /// Number of trials in a session
    int ntrials;

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

    bool load_experiment (const QString& path);

    bool exec ();

    /// Add a page to the composition of a trial
    void add_page (Page* page);

    void run_trial ();

    void show_page (int index);

    void next_page ();

    /// Define a special frame
    bool gen_frame (const std::string& frame_name);

    /// Clear the screen
    bool clear_screen ();

    int textureSize () const { return tex_size; }
    void setTextureSize (int size) { tex_size = size; }

    /// Convert a degrees distance to pixels
    Q_INVOKABLE float deg2pix (float dst) const {
      return 2*distance*tan(radians(dst)/2)*px_mm;
    }

    Q_INVOKABLE float ds2pf (float speed) const {
      return deg2pix (speed/refresh_edit->text ().toFloat ());
    }

    void set_glformat (QGLFormat glformat);

    float monitor_rate () const;

    Q_INVOKABLE void set_swap_interval (int swap_interval);

  public slots:

    /**
     * Run a session of multiple trials.
     * Sessions may run asynchronously, so this method may return
     * before the session being finished.
     */
    void run_session ();

  protected:
    void setup_updated ();

  protected slots:
    //void screen_param_changed ();
    void setup_param_changed ();
    void update_setup ();
    void update_converters ();
    void about_to_quit ();
    void quit ();
    void glwidget_gl_initialised ();
    void glwidget_key_press_event (QKeyEvent* evt);
    void gl_resized (int w, int h);
    void normal_screen_restored ();
    void script_engine_exception (const QScriptValue& exception);

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
    bool session_running;

    QByteArray splitter_state;
    QScriptEngine* script_engine;

    /**
     * Waiting for a fullscreen event before starting the session.
     */
    bool waiting_fullscreen;
  
    /// Random number generator
    mt19937 twister;
    /// Binary choice distribution
    uniform_int_distribution<int> bin_dist;
  };
}

#endif

// vim:filetype=cpp:
