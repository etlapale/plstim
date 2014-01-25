#include "client.h"


int
main (int argc, char* argv[])
{
    QCoreApplication app (argc, argv);

    plstim::Client client;

    return app.exec ();
}
