// plstim/engine.h – Manage a real experiment

#ifndef __PLSTIM_ENGINE_H
#define __PLSTIM_ENGINE_H

#include <random>
#include <set>

#include <QtQml>

// HDF5 C++ library
#include <H5Cpp.h>

#ifdef HAVE_EYELINK
#include <core_expt.h>
#include "elcalibration.h"
#endif

#ifdef HAVE_POWERMATE
#include "powermate.h"
#endif // HAVE_POWERMATE

#include "qtypes.h"
#include "setup.h"
#include "stimwindow.h"
#include "utils.h"


namespace plstim
{
class Engine : public QObject
{
    Q_OBJECT
    Q_PROPERTY (float monitor_rate READ monitor_rate)
    Q_PROPERTY (bool sessionRunning READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY (int currentTrial READ currentTrial WRITE setCurrentTrial NOTIFY currentTrialChanged)

public:
    const plstim::Setup* setup () const
    { return &m_setup; }

protected:
    /// Stimulus OpenGL window
    StimWindow* stim;

public:
    /// Application settings
    QSettings* settings;
protected:

    /// Current setup
    Setup m_setup;

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

    /// Accepted keys to switch to next page
    QSet<int> nextPageKeys;

  public:

    Engine ();

    virtual ~Engine ();

    bool load_experiment (const QString& path);

  public slots:
    void unloadExperiment ();

    void endSession ();

    void nextPage ();

  public:
    void run_trial ();

    void show_page (int index);

    float monitor_rate () const;

    //void set_swap_interval (int swap_interval);
    
    bool running () const
    { return m_running; }

    void setRunning (bool running)
    {
	m_running = running;
	emit runningChanged (running);
    }

    int currentTrial () const
    { return m_currentTrial; }

    void setCurrentTrial (int trial)
    {
	m_currentTrial = trial;
	emit currentTrialChanged (trial);
    }

  public slots:

    /**
     * Run a session of multiple trials.
     * Sessions may run asynchronously, so this method may return
     * before the session being finished.
     */
    void runSession ();
    void runSessionInline ();

    void set_trial_count (int ntrials);

  protected:
    void init_session ();

    void setup_updated ();

    void paintPage (Page* page, QImage& img, QPainter& painter);

    bool check_lua (int retcode);

    void savePageParameter (const QString& pageTitle,
	                    const QString& paramName,
			    int paramValue);

  protected:
    bool creating_subject;

  protected slots:
    void loadSetup (const QString& setupName);
    void updateRecents ();
    void about_to_quit ();
    void quit ();
    void stimKeyPressed (QKeyEvent* evt);
    //void stimScreenChanged (QScreen* screen);
#ifdef HAVE_POWERMATE
    void powerMateRotation (PowerMateEvent* evt);
    void powerMateButtonPressed (PowerMateEvent* evt);
#endif // HAVE_POWERMATE

    void selectSubject (const QString& subjectName);

  protected:
    bool m_running;
    int m_currentTrial;
    bool save_setup;

  public:
    QString xp_name;
  protected:
    int session_number;

    int max_recents;

    /// QML scripting engine to load the experiments
    QQmlEngine m_engine;
    /// QML component for the current experiment
    QQmlComponent* m_component;

  public:
    /// Currently loaded experiment
    plstim::Experiment* m_experiment;
  protected:

    /// Last opened directory in file dialog
    QString last_dialog_dir;

    QString last_datafile_dir;

  public:
  
    /// Random number generator
    std::mt19937 twister;

    /// Binary choice distribution
    std::uniform_int_distribution<int> bin_dist;

    /// Uniform double distribution in [0,1]
    std::uniform_real_distribution<double> real_dist;

    std::map<QString,int> key_mapping;

  public:

    /// Memory for the trial record
    void* trial_record;

    /// Memory size required by a trial record
    size_t record_size;

    /// Trial record offsets
    std::map<QString,size_t> trial_offsets;

    /// Page record offsets
    std::map<QString,std::map<QString,size_t>> record_offsets;

    /// Trial record types
    std::map<QString,H5::DataType> trial_types;

    // List of key accepted across the experiment
    std::set<QString> xp_keys;

    H5::CompType* record_type;

    H5::H5File* hf;
    H5::DataSet dset;

    QElapsedTimer timer;

  public:
    void error (const QString& msg, const QString& desc="");

#ifdef HAVE_EYELINK
  protected:
    bool eyelink_connected;
    bool eyelink_dummy;
    bool waiting_fixation;
  public:
    EyeLinkCalibrator* calibrator;
  public:
    void load_eyelink ();
    void calibrate_eyelink ();
    bool check_eyelink (INT16 errcode, const QString& func_name);
#endif

signals:
    void runningChanged (bool running);
    void currentTrialChanged (int trial);
};
}

#endif

// vim:filetype=cpp:
