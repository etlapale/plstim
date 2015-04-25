// src/client/client.cc – Client controller
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#include "client.h"


int
main (int argc, char* argv[])
{
    QCoreApplication app (argc, argv);

    plstim::Client client;

    return app.exec ();
}
