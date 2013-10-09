#include "gui.h"

using namespace plstim;


GUI::GUI (QWindow* parent)
    : QQuickView (parent)
{
    // Link setup to GUI
    rootContext ()->setContextProperty ("setup",
	    (plstim::Setup*) m_engine.setup ());
    rootContext ()->setContextProperty ("errorsModel",
	    QVariant::fromValue (m_engine.errors ()));

    // Load the QtQuick interface
    setSource (QUrl::fromLocalFile ("qml/ui.qml"));
    setFlags (Qt::Dialog);

    // Display machine information
    auto obj = rootObject ()->findChild<QObject*> ("timerParam");
    if (obj)
	obj->setProperty ("value",
		m_engine.timer.isMonotonic () ?
		"monotonic" : "non monotonic");
    obj = rootObject ()->findChild<QObject*> ("hostParam");
    if (obj)
	obj->setProperty ("value", QHostInfo::localHostName ());

    // Connect the QtQuick interface to the experiment
    obj = rootObject ()->findChild<QObject*> ("quitButton");
    if (obj)
	QObject::connect (obj, SIGNAL (buttonClick ()),
		&m_engine, SLOT (quit ()));
    obj = rootObject ()->findChild<QObject*> ("runButton");
    if (obj)
	QObject::connect (obj, SIGNAL (buttonClick ()),
		&m_engine, SLOT (runSession ()));
    obj = rootObject ()->findChild<QObject*> ("runInlineButton");
    if (obj)
	QObject::connect (obj, SIGNAL (buttonClick ()),
		&m_engine, SLOT (runSessionInline ()));
    obj = rootObject ()->findChild<QObject*> ("abortButton");
    if (obj)
	QObject::connect (obj, SIGNAL (buttonClick ()),
		&m_engine, SLOT (endSession ()));
    QObject::connect (rootObject (), SIGNAL (subjectSelected (const QString&)),
	    &m_engine, SLOT (selectSubject (const QString&)));
    
    // Dynamically update setup
    /*connect (&m_engine, &Engine::setupUpdated, [this] (Setup* setup) {
	    this->rootContext ()->setContextProperty ("setup", m_engine.setup ());
	});*/

    // Show experiment parameters
    connect (&m_engine, &Engine::experimentChanged, [this] (Experiment* experiment) {
	    auto obj = this->rootObject ()->findChild<QObject*> ("trialCount");
	    if (obj)
	      obj->setProperty ("value", experiment ? experiment->trialCount () : 0);
	});
    
    // Dynamically show action buttons
    connect (&m_engine, &Engine::runningChanged, [this] (bool running) {
	    this->rootObject ()->setProperty ("running", running);
	});

    // Display error messages
    connect (&m_engine, &Engine::errorsChanged,
	    [this] () {
		this->rootContext ()->setContextProperty ("errorsModel", QVariant::fromValue (m_engine.errors ()));
	    });

    // Show trial number in the current session block
    connect (&m_engine, &Engine::currentTrialChanged, [this] (int trial) {
	    auto obj = this->rootObject ()->findChild<QObject*> ("trialText");
	    if (obj)
		obj->setProperty ("text", QString ("%1/%2").arg (trial+1).arg (m_engine.m_experiment ? m_engine.m_experiment->trialCount () : 0));
	});
}

void
GUI::loadExperiment (const QString& path)
{
    m_engine.loadExperiment (path);
   
    // Update the subject list
    //auto settings = m_engine.settings;
    QStringList list;
    QJsonDocument json = m_engine.experimentDescription ();
    auto subjects = json.object ()["Subjects"];
    if (subjects.isObject ())
	list = subjects.toObject ().keys ();
    auto obj = rootObject ()->findChild<QObject*> ("subjectList");
    if (obj)
	obj->setProperty ("model", QVariant::fromValue (list));
}
