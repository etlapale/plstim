#include "gui.h"

using namespace plstim;


GUI::GUI (QWindow* parent)
    : QQuickView (parent)
{
    // Link setup to GUI
    rootContext ()->setContextProperty ("setup",
	    (plstim::Setup*) m_engine.setup ());

    // Load the QtQuick interface
    setSource (QUrl::fromLocalFile ("qml/ui.qml"));

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

    // Dynamically show action buttons
    connect (&m_engine, &Engine::runningChanged, [this] (bool running) {
	    this->rootObject ()->setProperty ("running", running);
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
    m_engine.load_experiment (path);
   
    // Update the subject list
    auto settings = m_engine.settings;
    QStringList list;
    settings->beginGroup (QString ("experiments/%1/subjects").arg (m_engine.xp_name));
    for (auto name : settings->childKeys ()) {
	list << name;
	qDebug () << "adding" << name;
    }
    settings->endGroup ();
    auto obj = rootObject ()->findChild<QObject*> ("subjectList");
    if (obj)
	obj->setProperty ("model", QVariant::fromValue (list));
}
