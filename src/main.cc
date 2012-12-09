#include "qexperiment.h"


int
main (int argc, char* argv[])
{
  plstim::QExperiment xp (argc, argv);
  qDebug () << xp.exec ();
  return 0;
}
