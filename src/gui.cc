#include "gui.h"

using namespace plstim;


GUI::GUI (QWindow* parent)
    : QQuickView (parent)
{
    // Load the QtQuick interface
    setSource (QUrl::fromLocalFile ("qml/ui.qml"));

    // Connect the QtQuick interface to the experiment
    auto obj = rootObject ()->findChild<QObject*> ("quitButton");
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

    // Dynamically show action buttons
    connect (&m_engine, &QExperiment::runningChanged, [this] (bool running) {
	    this->rootObject ()->setProperty ("running", running);
	});


    // Show trial number in the current session block
    connect (&m_engine, &QExperiment::currentTrialChanged, [this] (int trial) {
	    auto obj = this->rootObject ()->findChild<QObject*> ("trialText");
	    if (obj)
		obj->setProperty ("text", QString ("%1/%2").arg (trial+1).arg (m_engine.m_experiment ? m_engine.m_experiment->trialCount () : 0));
	});
}

void
GUI::loadExperiment (const QString& path)
{
    m_engine.load_experiment (path);
}
