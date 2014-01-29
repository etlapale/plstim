#include <QtGui>
#ifdef HAVE_EYELINK
#include <QtWidgets>
#endif // HAVE_EYELINK

#include "gui.h"
#ifdef WITH_NETWORK
#include "server.h"
#endif // WITH_NETWORK

#include "qtypes.h"

using namespace plstim;

int
main (int argc, char* argv[])
{
    // Qt application with a GUI
#ifdef HAVE_EYELINK
    QApplication app (argc, argv);
#else
    QGuiApplication app (argc, argv);
#endif

    // Create a window for PlStim
    plstim::GUI gui (QUrl("qrc:/qml/ui.qml"));

    // Load an experiment if given as command line argument
    auto args = app.arguments ();
    if (args.size () == 2) {
	gui.loadExperiment (args.at (1));
    }

#ifdef HAVE_POWERMATE
    // Watch for PowerMate events in a background thread
    PowerMateWatcher watcher;
    QThread powerMateThread;
    QObject::connect (&powerMateThread, SIGNAL (started ()),
	    &watcher, SLOT (watch ()));
    watcher.moveToThread (&powerMateThread);
    powerMateThread.start ();
#endif // HAVE_POWERMATE

#ifdef WITH_NETWORK
    plstim::Server server (gui.engine ());
    QThread serverThread;
    QObject::connect (&serverThread, SIGNAL (started ()),
	    &server, SLOT (start ()));
    server.moveToThread (&serverThread);
    serverThread.start ();
#endif // WITH_NETWORK

    // Run the application
    auto res = app.exec ();

    return res;
}
