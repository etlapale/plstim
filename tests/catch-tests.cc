#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <QtCore>
#include <QtQml>

#include "../lib/experiment.h"

int main(int argc, char* const argv[] )
{
  Catch::Session session;

  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0)
    return returnCode;

  QCoreApplication app(argc, const_cast<char**>(argv));

  qmlRegisterType<plstim::Experiment2> ("PlStim", 1, 0, "Experiment");
  
  return session.run();
}
