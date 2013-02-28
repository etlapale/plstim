#include <QtGui>
#include "gui.h"


int
main (int argc, char* argv[])
{
    // Qt application with a GUI
    QGuiApplication app (argc, argv);

    // Create a window for PlStim
    plstim::GUI gui;
    gui.show ();

    // Load an experiment if given as command line argument
    auto args = app.arguments ();
    if (args.size () == 2) {
	gui.loadExperiment (args.at (1));
    }

#ifdef HAVE_POWERMATE
    // Watch for PowerMate events in a background thread
    PowerMateWatcher watcher;
    QThread wthread;
    QObject::connect (&wthread, SIGNAL (started ()), &watcher, SLOT (watch ()));
    watcher.moveToThread (&wthread);
    wthread.start ();
#endif // HAVE_POWERMATE

    // Run the application
    return app.exec ();
}
