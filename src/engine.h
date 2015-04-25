// src/engine.h – Experiment engine
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <random>
#include <set>

#include <QtCore>
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
    Q_PROPERTY (bool sessionRunning READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY (int currentTrial READ currentTrial WRITE setCurrentTrial NOTIFY currentTrialChanged)
    Q_PROPERTY (int eta READ eta WRITE setEta NOTIFY etaChanged)
    Q_PROPERTY (QString subjectName READ subjectName WRITE setSubjectName NOTIFY subjectChanged)

public:
    plstim::Setup* setup ()
    { return &m_setup; }

    QSettings* settings ()
    { return m_settings; }

protected:
    /// Stimulus OpenGL window
    StimWindow* stim;

    QMetaObject::Connection m_exposed_conn;

    /// Application settings
    QSettings* m_settings;

    /// Current setup
    Setup m_setup;

    /// Experiment description
    QJsonDocument m_json;

    QList<QObject*> m_errors;

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

    QList<QObject*> errors ()
    { return m_errors; }

    bool loadExperiment (const QString& path);

    QJsonDocument& experimentDescription ()
    { return m_json; }

    /// Evaluate a JavaScript expression in the QML engine
    QVariant evaluate (const QString& expression);

  public slots:
    void unloadExperiment ();

    void endSession ();

    void nextPage (Page* wantedPage=NULL);

    Error* error (const QString& msg, const QString& description="");

  public:
    void run_trial ();

    void show_page (int index);

    bool isRunning () const
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

    int eta () const { return m_eta; }

    void setEta (int eta) { m_eta = eta; emit etaChanged (eta); }

    const QString& subjectName () const { return m_subjectName; }

    void setSubjectName (const QString& name) {
      m_subjectName = name;
      emit subjectChanged (name);
    }

    static QScreen* displayScreen ();

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

    void connectStimWindowExposed ();

    void savePageParameter (const QString& pageTitle,
	                    const QString& paramName,
			    int paramValue);

  protected slots:
    void loadSetup (const QString& setupName);
    void about_to_quit ();
    void quit ();
    void stimKeyPressed (QKeyEvent* evt);
    //void stimScreenChanged (QScreen* screen);
#ifdef HAVE_POWERMATE
    void powerMateRotation (PowerMateEvent* evt);
    void powerMateButtonPressed (PowerMateEvent* evt);
#endif // HAVE_POWERMATE

  public:
    void selectSubject (const QString& subjectName);

  protected:
    bool m_running;
    int m_currentTrial;
    bool save_setup;
    QString m_subjectName;

    /// Time at which the session started in ms since epoch
    qint64 m_sessionStart;

    /// Estimated remaining time for the session (in seconds)
    int m_eta;

    QMetaObject::Connection m_showPageCon;

  public:
    QString xp_name;
  protected:
    int session_number;

    /// QML scripting engine to load the experiments
    QQmlEngine m_engine;
    /// QML component for the current experiment
    QQmlComponent* m_component;

  public:
    /// Currently loaded experiment
    plstim::Experiment* m_experiment;

    plstim::Experiment* experiment () const { return m_experiment; }
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
    QSet<QString> xp_keys;

    H5::CompType* record_type;

    H5::H5File* hf;
    H5::DataSet dset;

    QElapsedTimer timer;

#ifdef HAVE_EYELINK
  protected:
    bool eyelink_connected;
    bool eyelink_dummy;
    bool waiting_fixation;
    bool m_eyelinkRecording;
  public:
    EyeLinkCalibrator* calibrator;
  public:
    void load_eyelink ();
    void calibrate_eyelink ();
    bool check_eyelink (INT16 errcode, const QString& func_name);
#endif // HAVE_EYELINK

signals:
    void errorsChanged ();
    void runningChanged (bool running);
    void currentTrialChanged (int trial);
    void etaChanged (int eta);
    void subjectChanged (const QString& subject);
    void experimentChanged (Experiment* experiment);
};

} // namespace plstim

// Local Variables:
// mode: c++
// End:

// vim:filetype=cpp:
