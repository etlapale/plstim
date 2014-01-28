#include "gui.h"

using namespace plstim;


GUI::GUI (const QUrl& uiUrl)
{
    // Load the QtQuick interface
    m_ui_engine.rootContext ()->setContextProperty ("xp",
        QVariant::fromValue ((QObject*) NULL));
    m_ui_engine.rootContext ()->setContextProperty ("setup",
            (plstim::Setup*) m_engine.setup ());
    m_ui_engine.rootContext ()->setContextProperty ("errorsModel",
            QVariant::fromValue (m_engine.errors ()));
    m_ui_engine.load (uiUrl);
    QObject* topLevel = m_ui_engine.rootObjects ().value (0);
    QQuickWindow* win = qobject_cast<QQuickWindow*> (topLevel);
    if (! win) {
        qCritical ("Root GUI item is not a window!");
        return;
    }

    // Make floatting and display
    win->setFlags (Qt::Dialog);
    win->show ();

    
    // Display machine information
    auto obj = topLevel->findChild<QObject*> ("timerParam");
    if (obj)
	obj->setProperty ("text",
		m_engine.timer.isMonotonic () ?
		"monotonic" : "non monotonic");
    obj = topLevel->findChild<QObject*> ("hostParam");
    if (obj)
	obj->setProperty ("text", QHostInfo::localHostName ());

    // Connect the QtQuick interface to the experiment
    obj = topLevel->findChild<QObject*> ("quitButton");
    if (obj)
	QObject::connect (obj, SIGNAL (clicked ()),
		&m_engine, SLOT (quit ()));
    obj = topLevel->findChild<QObject*> ("runButton");
    if (obj)
	QObject::connect (obj, SIGNAL (clicked ()),
		&m_engine, SLOT (runSession ()));
    obj = topLevel->findChild<QObject*> ("runInlineButton");
    if (obj)
	QObject::connect (obj, SIGNAL (clicked ()),
		&m_engine, SLOT (runSessionInline ()));
    obj = topLevel->findChild<QObject*> ("abortButton");
    if (obj)
	QObject::connect (obj, SIGNAL (clicked ()),
		&m_engine, SLOT (endSession ()));
    obj = topLevel->findChild<QObject*> ("subjectList");
    if (obj)
      QObject::connect (obj, SIGNAL (activated (int)),
          this, SLOT (subjectSelected (int)));
    
    // Dynamically update setup
    /*connect (&m_engine, &Engine::setupUpdated, [this] (Setup* setup) {
	    this->rootContext ()->setContextProperty ("setup", m_engine.setup ());
	});*/

    // Show experiment parameters
    QObject::connect (&m_engine, &Engine::experimentChanged, [topLevel] (Experiment* experiment) {
	    auto obj = topLevel->findChild<QObject*> ("trialCount");
	    if (obj)
	      obj->setProperty ("value", experiment ? experiment->trialCount () : 0);
	});
    
    // Dynamically show action buttons
    QObject::connect (&m_engine, &Engine::runningChanged, [topLevel] (bool running) {
	    topLevel->setProperty ("running", running);
	});

    // Display error messages
    QObject::connect (&m_engine, &Engine::errorsChanged,
	    [this] () {
		this->m_ui_engine.rootContext ()->setContextProperty ("errorsModel", QVariant::fromValue (m_engine.errors ()));
	    });

    // Show trial number in the current session block
    QObject::connect (&m_engine, &Engine::currentTrialChanged, [topLevel,this] (int trial) {
	    auto obj = topLevel->findChild<QObject*> ("trialText");
	    if (obj)
		obj->setProperty ("text", QString ("%1/%2").arg (trial+1).arg (m_engine.m_experiment ? m_engine.m_experiment->trialCount () : 0));
	});
}

template <typename T>
void
setChildProperty (QObject* root, const QString& childName,
                  const char* property, const T& value)
{
    auto obj = root->findChild<QObject*> (childName);
    if (obj != NULL)
        obj->setProperty (property, value);
}

void
GUI::loadExperiment (const QString& path)
{
    qDebug () << "Loading experiment from " << path;

    m_engine.loadExperiment (path);
    auto xp = m_engine.experiment ();
    QObject* topLevel = m_ui_engine.rootObjects ().value (0);

    // Update experiment info
    m_ui_engine.rootContext ()->setContextProperty ("xp", QVariant::fromValue (xp));
   
    // Update the subject list
    m_subjectList.clear ();
    m_subjectList << "None";
    QJsonDocument json = m_engine.experimentDescription ();
    auto subjects = json.object ()["Subjects"];
    if (subjects.isObject ())
	m_subjectList << subjects.toObject ().keys ();
    this->m_ui_engine.rootContext ()->setContextProperty ("errorsModel", QVariant::fromValue (m_engine.errors ()));
    auto obj = topLevel->findChild<QObject*> ("subjectList");
    if (obj)
	obj->setProperty ("model", QVariant::fromValue (m_subjectList));
}

void
GUI::subjectSelected (int index)
{
    m_engine.selectSubject (m_subjectList.at (index));
    // Search for subject parameters
    auto xp = m_engine.experiment ();
    auto& params = xp->subjectParameters ();
    QList<QObject*> paramList;
    for (auto key : params.keys ())
      paramList << new plstim::Parameter (QString(key), 123.456, "mm");
    // Update subject parameters
    QObject* topLevel = m_ui_engine.rootObjects ().value (0);
    auto obj = topLevel->findChild<QObject*> ("subjectParams");
    if (obj)
	obj->setProperty ("model", QVariant::fromValue (paramList));
}
