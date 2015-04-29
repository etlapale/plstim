#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <QtCore>
#include <QtQml>

#include "../lib/experiment.h"
#include "../lib/utils.h"

int main(int argc, char* const argv[] )
{
  Catch::Session session;

  int returnCode = session.applyCommandLine(argc, argv);
  if (returnCode != 0)
    return returnCode;

  QCoreApplication app(argc, const_cast<char**>(argv));

  plstim::initialise();
  
  return session.run();
}
